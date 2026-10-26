// Copyright 2026 kirzo

#include "KzGraphEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "SGraphNode.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "ScopedTransaction.h"

namespace
{
	TMap<TWeakObjectPtr<const UScriptStruct>, FKzOnMakeGraphNodeBody>& GetNodeBodies()
	{
		static TMap<TWeakObjectPtr<const UScriptStruct>, FKzOnMakeGraphNodeBody> NodeBodies;
		return NodeBodies;
	}

	TMap<TWeakObjectPtr<const UScriptStruct>, FKzOnGraphNodeDoubleClicked>& GetNodeDoubleClicks()
	{
		static TMap<TWeakObjectPtr<const UScriptStruct>, FKzOnGraphNodeDoubleClicked> NodeDoubleClicks;
		return NodeDoubleClicks;
	}

	/** The entry of the node's struct or its closest parent. */
	template <typename DelegateType>
	const DelegateType* FindForNode(const TMap<TWeakObjectPtr<const UScriptStruct>, DelegateType>& Map, const UKzEdGraphNode& Node)
	{
		for (const UStruct* Struct = Node.Node.GetScriptStruct(); Struct; Struct = Struct->GetSuperStruct())
		{
			if (const DelegateType* Found = Map.Find(Cast<UScriptStruct>(Struct)))
			{
				return Found;
			}
		}
		return nullptr;
	}

	void GatherTargets(const UEdGraphPin& Output, EKzGraphFlow Flow, TArray<UEdGraphPin*>& OutTargets, TSet<const UEdGraphNode*>& Visited)
	{
		TArray<UEdGraphPin*> Linked = Output.LinkedTo;
		Linked.RemoveAll([](const UEdGraphPin* Pin) { return !Pin; });
		KzGraphEditor::SortByFlow(Linked, Flow);

		for (UEdGraphPin* Pin : Linked)
		{
			const UEdGraphNode* Node = Pin->GetOwningNode();
			if (!Node->IsA<UKzEdGraphNode_Reroute>())
			{
				OutTargets.Add(Pin);
				continue;
			}

			bool bAlreadyVisited = false;
			Visited.Add(Node, &bAlreadyVisited);
			if (!bAlreadyVisited)
			{
				GatherTargets(*Node->Pins[1], Flow, OutTargets, Visited);
			}
		}
	}
}

void KzGraphEditor::RegisterNodeBody(const UScriptStruct* Struct, FKzOnMakeGraphNodeBody MakeBody)
{
	GetNodeBodies().Add(Struct, MoveTemp(MakeBody));
}

void KzGraphEditor::UnregisterNodeBody(const UScriptStruct* Struct)
{
	GetNodeBodies().Remove(Struct);
}

TSharedPtr<SWidget> KzGraphEditor::MakeNodeBody(const UKzEdGraphNode& Node)
{
	const FKzOnMakeGraphNodeBody* MakeBody = FindForNode(GetNodeBodies(), Node);
	return MakeBody && MakeBody->IsBound() ? MakeBody->Execute(Node) : nullptr;
}

void KzGraphEditor::RegisterNodeDoubleClick(const UScriptStruct* Struct, FKzOnGraphNodeDoubleClicked OnDoubleClicked)
{
	GetNodeDoubleClicks().Add(Struct, MoveTemp(OnDoubleClicked));
}

void KzGraphEditor::UnregisterNodeDoubleClick(const UScriptStruct* Struct)
{
	GetNodeDoubleClicks().Remove(Struct);
}

void KzGraphEditor::DoubleClickNode(const UKzEdGraphNode& Node)
{
	if (const FKzOnGraphNodeDoubleClicked* OnDoubleClicked = FindForNode(GetNodeDoubleClicks(), Node))
	{
		OnDoubleClicked->ExecuteIfBound(Node);
	}
}

UKzGraph* KzGraphEditor::GetGraphAsset(const UEdGraph* Graph)
{
	return Graph ? Graph->GetTypedOuter<UKzGraph>() : nullptr;
}

void KzGraphEditor::SortByFlow(TArray<UEdGraphPin*>& Pins, EKzGraphFlow Flow)
{
	Pins.StableSort([Flow](const UEdGraphPin& A, const UEdGraphPin& B)
	{
		const UEdGraphNode* NodeA = A.GetOwningNode();
		const UEdGraphNode* NodeB = B.GetOwningNode();
		const FIntPoint PosA = Flow == EKzGraphFlow::Horizontal ? FIntPoint(NodeA->NodePosY, NodeA->NodePosX) : FIntPoint(NodeA->NodePosX, NodeA->NodePosY);
		const FIntPoint PosB = Flow == EKzGraphFlow::Horizontal ? FIntPoint(NodeB->NodePosY, NodeB->NodePosX) : FIntPoint(NodeB->NodePosX, NodeB->NodePosY);
		return PosA.X != PosB.X ? PosA.X < PosB.X : PosA.Y < PosB.Y;
	});
}

void KzGraphEditor::GatherTargets(const UEdGraphPin& Output, EKzGraphFlow Flow, TArray<UEdGraphPin*>& OutTargets)
{
	TSet<const UEdGraphNode*> Visited;
	::GatherTargets(Output, Flow, OutTargets, Visited);
}

void KzGraphEditor::StraightenConnections(SGraphEditor& Editor)
{
	const UKzGraph* Asset = GetGraphAsset(Editor.GetCurrentGraph());
	if (!Asset || Asset->GetFlow() != EKzGraphFlow::Vertical)
	{
		Editor.OnStraightenConnections();
		return;
	}

	// Each selected node moves sideways until its input sits under the output feeding it; top to bottom, so chains follow.
	SGraphPanel* Panel = Editor.GetGraphPanel();
	TArray<UEdGraphNode*> Selected;
	for (UObject* Object : Editor.GetSelectedNodes())
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			Selected.Add(Node);
		}
	}
	Selected.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodePosY < B.NodePosY; });

	const FScopedTransaction Transaction(FGraphEditorCommands::Get().StraightenConnections->GetLabel());
	TSet<const UEdGraphNode*> Moved;
	for (UEdGraphNode* Source : Selected)
	{
		const TSharedPtr<SGraphNode> SourceWidget = Panel ? Panel->GetNodeWidgetFromGuid(Source->NodeGuid) : nullptr;
		if (!SourceWidget.IsValid())
		{
			continue;
		}

		for (UEdGraphPin* Output : Source->Pins)
		{
			if (Output->Direction != EGPD_Output)
			{
				continue;
			}

			for (UEdGraphPin* Input : Output->LinkedTo)
			{
				UEdGraphNode* Target = Input ? Input->GetOwningNode() : nullptr;
				if (!Target || Target == Source || Moved.Contains(Target) || !Selected.Contains(Target))
				{
					continue;
				}

				const TSharedPtr<SGraphNode> TargetWidget = Panel->GetNodeWidgetFromGuid(Target->NodeGuid);
				const TSharedPtr<SGraphPin> OutputWidget = SourceWidget->FindWidgetForPin(Output);
				const TSharedPtr<SGraphPin> InputWidget = TargetWidget.IsValid() ? TargetWidget->FindWidgetForPin(Input) : nullptr;
				if (!OutputWidget.IsValid() || !InputWidget.IsValid())
				{
					continue;
				}

				const float OutputX = Source->NodePosX + OutputWidget->GetNodeOffset().X + OutputWidget->GetDesiredSize().X * 0.5f;
				const float InputX = InputWidget->GetNodeOffset().X + InputWidget->GetDesiredSize().X * 0.5f;
				Target->GetSchema()->SetNodePosition(Target, FVector2f(OutputX - InputX, Target->NodePosY));
				Moved.Add(Target);
			}
		}
	}
}
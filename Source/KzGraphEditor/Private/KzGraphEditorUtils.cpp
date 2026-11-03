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
#include "StructUtils/InstancedStruct.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Moves the node of Moving sideways until Moving's center lines up with Anchor's, down a vertical flow. */
	bool AlignPins(SGraphPanel& Panel, UEdGraphPin& Anchor, UEdGraphPin& Moving)
	{
		UEdGraphNode* AnchorNode = Anchor.GetOwningNode();
		UEdGraphNode* MovingNode = Moving.GetOwningNode();
		const TSharedPtr<SGraphNode> AnchorWidget = Panel.GetNodeWidgetFromGuid(AnchorNode->NodeGuid);
		const TSharedPtr<SGraphNode> MovingWidget = Panel.GetNodeWidgetFromGuid(MovingNode->NodeGuid);
		const TSharedPtr<SGraphPin> AnchorPin = AnchorWidget.IsValid() ? AnchorWidget->FindWidgetForPin(&Anchor) : nullptr;
		const TSharedPtr<SGraphPin> MovingPin = MovingWidget.IsValid() ? MovingWidget->FindWidgetForPin(&Moving) : nullptr;
		if (!AnchorPin.IsValid() || !MovingPin.IsValid())
		{
			return false;
		}

		const float AnchorX = AnchorNode->NodePosX + AnchorPin->GetNodeOffset().X + AnchorPin->GetDesiredSize().X * 0.5f;
		const float MovingX = MovingPin->GetNodeOffset().X + MovingPin->GetDesiredSize().X * 0.5f;
		MovingNode->GetSchema()->SetNodePosition(MovingNode, FVector2f(AnchorX - MovingX, MovingNode->NodePosY));
		return true;
	}

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

	void DuplicateObjects(const FProperty* Property, void* Value, UObject& Outer, TFunctionRef<bool(const FObjectProperty&, const UObject&)> ShouldCopy)
	{
		if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);
			if (Object && ShouldCopy(*ObjectProperty, *Object))
			{
				ObjectProperty->SetObjectPropertyValue(Value, DuplicateObject(Object, &Outer));
			}
		}
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == FInstancedStruct::StaticStruct())
			{
				FInstancedStruct& Instanced = *static_cast<FInstancedStruct*>(Value);
				if (Instanced.IsValid())
				{
					KzGraphEditor::DuplicateObjects(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), Outer, ShouldCopy);
				}
			}
			else
			{
				KzGraphEditor::DuplicateObjects(StructProperty->Struct, Value, Outer, ShouldCopy);
			}
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);
			for (int32 Index = 0; Index < Array.Num(); ++Index)
			{
				DuplicateObjects(ArrayProperty->Inner, Array.GetRawPtr(Index), Outer, ShouldCopy);
			}
		}
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
	// Modules can shut down after UObjects, when even looking up a weak pointer asserts
	if (UObjectInitialized())
	{
		GetNodeBodies().Remove(Struct);
	}
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
	if (UObjectInitialized())
	{
		GetNodeDoubleClicks().Remove(Struct);
	}
}

void KzGraphEditor::DoubleClickNode(const UKzEdGraphNode& Node)
{
	if (const FKzOnGraphNodeDoubleClicked* OnDoubleClicked = FindForNode(GetNodeDoubleClicks(), Node))
	{
		OnDoubleClicked->ExecuteIfBound(Node);
	}
}

void KzGraphEditor::DuplicateObjects(const UStruct* Struct, void* Memory, UObject& Outer, TFunctionRef<bool(const FObjectProperty&, const UObject&)> ShouldCopy)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		for (int32 Index = 0; Index < It->ArrayDim; ++Index)
		{
			::DuplicateObjects(*It, It->ContainerPtrToValuePtr<void>(Memory, Index), Outer, ShouldCopy);
		}
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

	SGraphPanel* Panel = Editor.GetGraphPanel();
	if (!Panel)
	{
		return;
	}

	// Each selected node moves sideways until its input sits under the output feeding it; top to bottom, so chains follow.
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
		for (UEdGraphPin* Output : Source->Pins)
		{
			if (Output->Direction != EGPD_Output)
			{
				continue;
			}

			for (UEdGraphPin* Input : Output->LinkedTo)
			{
				UEdGraphNode* Target = Input ? Input->GetOwningNode() : nullptr;
				if (Target && Target != Source && !Moved.Contains(Target) && Selected.Contains(Target) && AlignPins(*Panel, *Output, *Input))
				{
					Moved.Add(Target);
				}
			}
		}
	}
}

void KzGraphEditor::StraightenPinConnections(SGraphEditor& Editor, UEdGraphPin& Pin, UEdGraphPin* OnlyTo)
{
	SGraphPanel* Panel = Editor.GetGraphPanel();
	if (!Panel)
	{
		return;
	}

	const FScopedTransaction Transaction(FGraphEditorCommands::Get().StraightenConnections->GetLabel());
	const TArray<UEdGraphPin*> Linked = OnlyTo ? TArray<UEdGraphPin*>{ OnlyTo } : Pin.LinkedTo;
	for (UEdGraphPin* Other : Linked)
	{
		if (Other && Other->GetOwningNode() != Pin.GetOwningNode())
		{
			AlignPins(*Panel, Pin, *Other);
		}
	}
}
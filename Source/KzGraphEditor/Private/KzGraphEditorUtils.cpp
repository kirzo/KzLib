// Copyright 2026 kirzo

#include "KzGraphEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"

namespace
{
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
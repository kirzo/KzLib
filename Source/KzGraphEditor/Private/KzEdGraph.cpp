// Copyright 2026 kirzo

#include "KzEdGraph.h"
#include "Containers/Ticker.h"
#include "KzEdGraphSchema.h"
#include "KzGraph.h"

UKzEdGraph* UKzEdGraph::CreateFor(UKzGraph& Graph)
{
	UKzEdGraph* EdGraph = NewObject<UKzEdGraph>(&Graph, NAME_None, RF_Transactional);
	EdGraph->Schema = UKzEdGraphSchema::StaticClass();
	Graph.Modify();
	Graph.EdGraph = EdGraph;
	EdGraph->GetSchema()->CreateDefaultNodesForGraph(*EdGraph);
	return EdGraph;
}

UKzGraph* UKzEdGraph::GetGraphAsset() const
{
	return GetTypedOuter<UKzGraph>();
}

void UKzEdGraph::NotifyGraphChanged()
{
	MarkChanged();
	Super::NotifyGraphChanged();
}

void UKzEdGraph::NotifyGraphChanged(const FEdGraphEditAction& Action)
{
	if (!(Action.Action & GRAPHACTION_SelectNode))
	{
		MarkChanged();
	}
	Super::NotifyGraphChanged(Action);
}

void UKzEdGraph::PostEditUndo()
{
	Super::PostEditUndo();
	NotifyGraphChanged();
}

void UKzEdGraph::NotifyGraphChangedSoon()
{
	MarkChanged();
	if (bNotifyPending)
	{
		return;
	}

	bNotifyPending = true;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		bNotifyPending = false;
		NotifyGraphChanged();
		return false;
	}));
}

void UKzEdGraph::ReconstructNodes()
{
	for (UEdGraphNode* Node : Nodes)
	{
		Node->ReconstructNode();
	}
}
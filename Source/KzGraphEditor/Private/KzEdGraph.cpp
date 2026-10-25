// Copyright 2026 kirzo

#include "KzEdGraph.h"
#include "Containers/Ticker.h"
#include "KzEdGraphSchema.h"
#include "KzGraph.h"
#include "KzGraphCompiler.h"
#include "Logging/TokenizedMessage.h"

UKzEdGraph* UKzEdGraph::CreateFor(UKzGraph& Graph)
{
	UKzEdGraph* EdGraph = NewObject<UKzEdGraph>(&Graph, NAME_None, RF_Transactional);
	EdGraph->Schema = UKzEdGraphSchema::StaticClass();
	Graph.Modify();
	Graph.EdGraph = EdGraph;
	EdGraph->GetSchema()->CreateDefaultNodesForGraph(*EdGraph);
	EdGraph->Compile();
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

void UKzEdGraph::Compile()
{
	UKzGraph* Asset = GetGraphAsset();
	if (!Asset)
	{
		return;
	}

	FKzGraphData Data;
	CompileIssues.Reset();
	KzGraphCompiler::Compile(*this, *Asset, Data, CompileIssues);

	for (UEdGraphNode* Node : Nodes)
	{
		Node->bHasCompilerMessage = false;
		Node->ErrorType = EMessageSeverity::Info;
		Node->ErrorMsg.Reset();
	}

	// Each node shows its own problems; the worst sets the color.
	for (const FKzValidationIssue& Issue : CompileIssues)
	{
		const TObjectPtr<UEdGraphNode>* Node = Nodes.FindByPredicate([&Issue](const UEdGraphNode* Each) { return Each->NodeGuid == Issue.ContextId; });
		if (!Node || Issue.Severity == EKzValidationSeverity::Info)
		{
			continue;
		}

		const EMessageSeverity::Type Severity = Issue.Severity == EKzValidationSeverity::Error ? EMessageSeverity::Error : EMessageSeverity::Warning;
		(*Node)->bHasCompilerMessage = true;
		(*Node)->ErrorType = FMath::Min<int32>((*Node)->ErrorType, Severity);
		(*Node)->ErrorMsg += ((*Node)->ErrorMsg.IsEmpty() ? FString() : FString(TEXT("\n"))) + Issue.Message.ToString();
	}

	Asset->Modify();
	Asset->SetData(MoveTemp(Data));
	bNeedsCompile = false;

	// Node widgets read their problems when they rebuild.
	Super::NotifyGraphChanged();
	OnCompiled.Broadcast();
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
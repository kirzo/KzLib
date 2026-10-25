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

	// Each node shows its own problems; the worst sets the color.
	TMap<FGuid, TPair<int32, FString>> Problems;
	for (const FKzValidationIssue& Issue : CompileIssues)
	{
		if (Issue.Severity == EKzValidationSeverity::Info || !Issue.ContextId.IsValid())
		{
			continue;
		}

		TPair<int32, FString>& Problem = Problems.FindOrAdd(Issue.ContextId, { EMessageSeverity::Info, FString() });
		Problem.Key = FMath::Min<int32>(Problem.Key, Issue.Severity == EKzValidationSeverity::Error ? EMessageSeverity::Error : EMessageSeverity::Warning);
		Problem.Value += (Problem.Value.IsEmpty() ? FString() : FString(TEXT("\n"))) + Issue.Message.ToString();
	}

	bool bProblemsChanged = false;
	for (UEdGraphNode* Node : Nodes)
	{
		const TPair<int32, FString>* Problem = Problems.Find(Node->NodeGuid);
		const int32 ErrorType = Problem ? Problem->Key : EMessageSeverity::Info;
		const FString& ErrorMsg = Problem ? Problem->Value : FString();
		if (Node->bHasCompilerMessage != (Problem != nullptr) || Node->ErrorType != ErrorType || Node->ErrorMsg != ErrorMsg)
		{
			Node->bHasCompilerMessage = Problem != nullptr;
			Node->ErrorType = ErrorType;
			Node->ErrorMsg = ErrorMsg;
			bProblemsChanged = true;
		}
	}

	Asset->Modify();
	Asset->SetData(MoveTemp(Data));
	bNeedsCompile = false;

	// Node widgets read their problems when they rebuild, which a large graph pays for.
	if (bProblemsChanged)
	{
		Super::NotifyGraphChanged();
	}
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
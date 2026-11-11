// Copyright 2026 kirzo

#include "KzEdGraph.h"
#include "Containers/Ticker.h"
#include "GraphEditAction.h"
#include "KzEdGraphNode.h"
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
	EdGraph->SyncRequiredEntries();
	EdGraph->Compile();
	return EdGraph;
}

UKzGraph* UKzEdGraph::GetGraphAsset() const
{
	return GetTypedOuter<UKzGraph>();
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
	MarkChanged();
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

	// The panel's pin widgets still point at the pins just thrown away.
	NotifyGraphChanged();
}

bool UKzEdGraph::SyncRequiredEntries()
{
	const UKzGraph* Asset = GetGraphAsset();
	const UScriptStruct* EntryStruct = Asset ? Asset->GetRequiredEntryStruct() : nullptr;
	TArray<FName> Required;
	if (!EntryStruct || !Asset->GetRequiredEntries(Required))
	{
		return false;
	}

	// The nodes already serving a required entry, and the rest of their struct, free to take a missing one.
	TMap<FName, UKzEdGraphNode*> Serving;
	TArray<UKzEdGraphNode*> Spare;
	const bool bHorizontal = Asset->GetFlow() == EKzGraphFlow::Horizontal;
	FVector2f Next = FVector2f::ZeroVector;
	bool bPlaced = false;
	for (UEdGraphNode* Each : Nodes)
	{
		UKzEdGraphNode* Node = Cast<UKzEdGraphNode>(Each);
		if (!Node || Node->Node.GetScriptStruct() != EntryStruct)
		{
			continue;
		}

		const FName Name = Node->Node.Get().GetEntryName();
		if (Required.Contains(Name) && !Serving.Contains(Name))
		{
			Serving.Add(Name, Node);
		}
		else
		{
			Spare.Add(Node);
		}

		// New entries go after the last one, across the flow.
		const FVector2f Position(Node->NodePosX, Node->NodePosY);
		if (!bPlaced || (bHorizontal ? Position.Y > Next.Y : Position.X > Next.X))
		{
			Next = Position;
			bPlaced = true;
		}
	}

	constexpr float Spacing = 150.0f;
	if (bPlaced)
	{
		(bHorizontal ? Next.Y : Next.X) += Spacing;
	}

	bool bChanged = false;
	for (const FName Name : Required)
	{
		if (Serving.Contains(Name))
		{
			continue;
		}

		Modify();
		UKzEdGraphNode* Node = nullptr;
		if (!Spare.IsEmpty())
		{
			Node = Spare[0];
			Spare.RemoveAt(0);
			Node->Modify();
		}
		else
		{
			Node = UKzEdGraphSchema::CreateNode(*this, EntryStruct, Next, false);
			(bHorizontal ? Next.Y : Next.X) += Spacing;
		}

		Asset->SetRequiredEntry(FStructView(Node->Node.GetScriptStruct(), Node->Node.GetMutableMemory()), Name);
		Node->ReconstructNode();
		Serving.Add(Name, Node);
		bChanged = true;
	}

	for (UKzEdGraphNode* Node : Spare)
	{
		Modify();
		Node->Modify();
		RemoveNode(Node);
		bChanged = true;
	}

	if (bChanged)
	{
		MarkChanged();
		NotifyGraphChanged();
	}
	return bChanged;
}
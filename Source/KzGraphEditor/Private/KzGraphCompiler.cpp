// Copyright 2026 kirzo

#include "KzGraphCompiler.h"
#include "KzEdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "KzGraphCompiler"

const FName KzGraphCompiler::ValidatorId(TEXT("Compile"));

namespace
{
	void DuplicateOwnedObjects(const UStruct* Struct, void* Memory, const UObject& Source, UObject& Outer);

	/** Objects the editor node owns, such as instanced conditions, get a copy of their own in the asset. */
	void DuplicateOwnedObjects(const FProperty* Property, void* Value, const UObject& Source, UObject& Outer)
	{
		if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);
			if (Object && Object->IsIn(&Source))
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
					DuplicateOwnedObjects(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), Source, Outer);
				}
			}
			else
			{
				DuplicateOwnedObjects(StructProperty->Struct, Value, Source, Outer);
			}
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);
			for (int32 Index = 0; Index < Array.Num(); ++Index)
			{
				DuplicateOwnedObjects(ArrayProperty->Inner, Array.GetRawPtr(Index), Source, Outer);
			}
		}
	}

	void DuplicateOwnedObjects(const UStruct* Struct, void* Memory, const UObject& Source, UObject& Outer)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			for (int32 Index = 0; Index < It->ArrayDim; ++Index)
			{
				DuplicateOwnedObjects(*It, It->ContainerPtrToValuePtr<void>(Memory, Index), Source, Outer);
			}
		}
	}

	struct FNodePins
	{
		TArray<FKzGraphPinDesc> Inputs;
		TArray<FKzGraphPinDesc> Outputs;
	};
}

void KzGraphCompiler::Compile(const UKzEdGraph& Graph, UObject& Outer, FKzGraphData& OutData, TArray<FKzValidationIssue>& OutIssues)
{
	const UKzGraph* Asset = Graph.GetGraphAsset();
	if (!Asset)
	{
		return;
	}

	const EKzGraphFlow Flow = Asset->GetFlow();
	auto AddIssue = [&OutIssues](EKzValidationSeverity Severity, const FText& Message, const UEdGraphNode& Node)
	{
		OutIssues.Add(FKzValidationIssue::WithContextId(Severity, Message, ValidatorId, Node.NodeGuid));
	};

	TArray<const UKzEdGraphNode*> Candidates;
	TArray<const UKzEdGraphNode*> EntryNodes;
	for (const UEdGraphNode* GraphNode : Graph.Nodes)
	{
		const UKzEdGraphNode* Node = Cast<UKzEdGraphNode>(GraphNode);
		if (!Node)
		{
			continue;
		}

		if (!Node->Node.IsValid() || !Node->Node.GetScriptStruct()->IsChildOf(Asset->GetNodeStruct()))
		{
			AddIssue(EKzValidationSeverity::Error, LOCTEXT("InvalidNode", "This graph does not take this node; it is left out."), *Node);
			continue;
		}

		Candidates.Add(Node);
		if (!Node->Node.Get().GetEntryName().IsNone())
		{
			EntryNodes.Add(Node);
		}
	}

	// With entries, only what they reach is compiled.
	const TSet<const UKzEdGraphNode*> CandidateSet(Candidates);
	TSet<const UKzEdGraphNode*> Reached;
	TArray<const UKzEdGraphNode*> Pending = EntryNodes;
	while (!Pending.IsEmpty())
	{
		const UKzEdGraphNode* Node = Pending.Pop();
		bool bAlreadyReached = false;
		Reached.Add(Node, &bAlreadyReached);
		if (bAlreadyReached)
		{
			continue;
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction != EGPD_Output)
			{
				continue;
			}

			TArray<UEdGraphPin*> Targets;
			KzGraphEditor::GatherTargets(*Pin, Flow, Targets);
			for (const UEdGraphPin* Target : Targets)
			{
				if (const UKzEdGraphNode* TargetNode = Cast<UKzEdGraphNode>(Target->GetOwningNode()); TargetNode && CandidateSet.Contains(TargetNode))
				{
					Pending.Add(TargetNode);
				}
			}
		}
	}

	TArray<const UKzEdGraphNode*> Included;
	for (const UKzEdGraphNode* Node : Candidates)
	{
		if (EntryNodes.IsEmpty() || Reached.Contains(Node))
		{
			Included.Add(Node);
		}
		else
		{
			AddIssue(EKzValidationSeverity::Warning, LOCTEXT("Unreachable", "No entry reaches this node; it is left out."), *Node);
		}
	}

	TMap<const UEdGraphNode*, int32> Indices;
	TArray<FNodePins> Pins;
	TArray<FInstancedStruct> Nodes;
	for (const UKzEdGraphNode* Node : Included)
	{
		Indices.Add(Node, Nodes.Num());

		FNodePins& NodePins = Pins.AddDefaulted_GetRef();
		Node->Node.Get().GetInputPins(*Asset, NodePins.Inputs);
		Node->Node.Get().GetOutputPins(*Asset, NodePins.Outputs);

		FInstancedStruct& Copy = Nodes.AddDefaulted_GetRef();
		Copy.InitializeAs(Node->Node.GetScriptStruct(), Node->Node.GetMemory());
		DuplicateOwnedObjects(Copy.GetScriptStruct(), Copy.GetMutableMemory(), *Node, Outer);
	}

	OutData = FKzGraphData();
	OutData.Nodes = Nodes;

	const int32 MaxPerOutput = Asset->GetMaxLinksPerOutput();
	const int32 MaxPerInput = Asset->GetMaxLinksPerInput();
	TMap<TPair<int32, int32>, int32> LinksPerInput;

	for (int32 NodeIndex = 0; NodeIndex < Included.Num(); ++NodeIndex)
	{
		const UKzEdGraphNode& Node = *Included[NodeIndex];
		const FKzGraphNode& NodeStruct = Node.Node.Get();

		FKzGraphNodeRanges& Range = OutData.Ranges.AddDefaulted_GetRef();
		Range.FirstInput = OutData.Inputs.Num();
		Range.NumInputs = Pins[NodeIndex].Inputs.Num();
		for (const FKzGraphPinDesc& Input : Pins[NodeIndex].Inputs)
		{
			OutData.Inputs.Add(Input.Name);
		}

		Range.FirstOutput = OutData.Outputs.Num();
		Range.NumOutputs = Pins[NodeIndex].Outputs.Num();
		for (const FKzGraphPinDesc& OutputDesc : Pins[NodeIndex].Outputs)
		{
			FKzGraphOutput& Output = OutData.Outputs.AddDefaulted_GetRef();
			Output.Name = OutputDesc.Name;
			Output.FirstLink = OutData.Links.Num();

			TArray<UEdGraphPin*> Targets;
			if (const UEdGraphPin* Pin = Node.FindPin(OutputDesc.Name, EGPD_Output))
			{
				KzGraphEditor::GatherTargets(*Pin, Flow, Targets);
			}

			for (const UEdGraphPin* Target : Targets)
			{
				const int32* TargetIndex = Indices.Find(Target->GetOwningNode());
				if (!TargetIndex)
				{
					continue;
				}

				const int32 InputIndex = Pins[*TargetIndex].Inputs.IndexOfByPredicate([Target](const FKzGraphPinDesc& Input) { return Input.Name == Target->PinName; });
				if (InputIndex == INDEX_NONE)
				{
					continue;
				}

				FText Reason;
				const FConstStructView TargetStruct = Nodes[*TargetIndex];
				if (!Asset->CanConnect(Nodes[NodeIndex], OutputDesc.Name, TargetStruct, Target->PinName, Reason))
				{
					AddIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("CannotConnect", "{0} cannot link to {1}: {2}"), FText::FromName(OutputDesc.Name), Included[*TargetIndex]->GetTitle(), Reason), Node);
					continue;
				}

				OutData.Links.Add({ *TargetIndex, InputIndex });
				++LinksPerInput.FindOrAdd(TPair<int32, int32>(*TargetIndex, InputIndex));
			}

			Output.NumLinks = OutData.Links.Num() - Output.FirstLink;
			if (MaxPerOutput > 0 && Output.NumLinks > MaxPerOutput)
			{
				AddIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("TooManyOutputLinks", "{0} leads to {1} nodes; this graph takes {2} at most."), FText::FromName(OutputDesc.Name), Output.NumLinks, MaxPerOutput), Node);
			}
		}

		const FName EntryName = NodeStruct.GetEntryName();
		if (!EntryName.IsNone())
		{
			if (OutData.FindEntry(EntryName) != INDEX_NONE)
			{
				AddIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("DuplicateEntry", "Another node is already the entry {0}."), FText::FromName(EntryName)), Node);
			}
			else
			{
				OutData.Entries.Add({ EntryName, NodeIndex });
			}
		}

		TArray<FKzValidationIssue> NodeIssues;
		NodeStruct.Validate(NodeIssues);
		for (FKzValidationIssue& Issue : NodeIssues)
		{
			Issue.ContextId = Node.NodeGuid;
			Issue.ValidatorId = Issue.ValidatorId.IsNone() ? ValidatorId : Issue.ValidatorId;
			OutIssues.Add(MoveTemp(Issue));
		}

		OutData.NodeGuids.Add(Node.NodeGuid);
	}

	if (MaxPerInput > 0)
	{
		for (const TPair<TPair<int32, int32>, int32>& Count : LinksPerInput)
		{
			if (Count.Value > MaxPerInput)
			{
				const int32 NodeIndex = Count.Key.Key;
				AddIssue(EKzValidationSeverity::Error, FText::Format(LOCTEXT("TooManyInputLinks", "{0} is reached by {1} links; this graph takes {2} at most."), FText::FromName(OutData.GetInputName(NodeIndex, Count.Key.Value)), Count.Value, MaxPerInput), *Included[NodeIndex]);
			}
		}
	}

	const int32 FirstGraphIssue = OutIssues.Num();
	Asset->ValidateGraph(OutData, OutIssues);
	for (int32 Index = FirstGraphIssue; Index < OutIssues.Num(); ++Index)
	{
		FKzValidationIssue& Issue = OutIssues[Index];
		Issue.ValidatorId = Issue.ValidatorId.IsNone() ? ValidatorId : Issue.ValidatorId;
	}
}

#undef LOCTEXT_NAMESPACE
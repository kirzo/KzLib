// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzGraphNode.h"
#include "StructUtils/InstancedStructContainer.h"
#include "StructUtils/StructView.h"
#include "KzGraph.generated.h"

class UEdGraph;

/** Where a link goes: a node and one of its inputs. */
USTRUCT()
struct FKzGraphLink
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Node = INDEX_NONE;

	UPROPERTY()
	int32 Input = INDEX_NONE;
};

/** An output of a node and its links, in order. */
USTRUCT()
struct FKzGraphOutput
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	int32 FirstLink = 0;

	UPROPERTY()
	int32 NumLinks = 0;
};

/** Where the inputs and outputs of a node start in the flat arrays. */
USTRUCT()
struct FKzGraphNodeRanges
{
	GENERATED_BODY()

	UPROPERTY()
	int32 FirstInput = 0;

	UPROPERTY()
	int32 NumInputs = 0;

	UPROPERTY()
	int32 FirstOutput = 0;

	UPROPERTY()
	int32 NumOutputs = 0;
};

/** A node a runner can start from, by name. */
USTRUCT()
struct FKzGraphEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	int32 Node = INDEX_NONE;
};

/** A compiled graph: the nodes in one block of memory and their links in flat arrays, read by index. */
USTRUCT()
struct KZGRAPH_API FKzGraphData
{
	GENERATED_BODY()

	int32 NumNodes() const { return Nodes.Num(); }
	FConstStructView GetNode(int32 Node) const { return Nodes[Node]; }

	template<typename T>
	const T* GetNode(int32 Node) const { return Nodes[Node].GetPtr<const T>(); }

	TConstArrayView<FKzGraphEntry> GetEntries() const { return Entries; }

	/** The node an entry starts at, or INDEX_NONE. */
	int32 FindEntry(FName Name) const;

	int32 NumInputs(int32 Node) const { return Ranges[Node].NumInputs; }
	FName GetInputName(int32 Node, int32 Input) const { return Inputs[Ranges[Node].FirstInput + Input]; }

	int32 NumOutputs(int32 Node) const { return Ranges[Node].NumOutputs; }
	FName GetOutputName(int32 Node, int32 Output) const { return Outputs[Ranges[Node].FirstOutput + Output].Name; }

	/** Index of the output among those of the node, or INDEX_NONE. */
	int32 FindOutput(int32 Node, FName Name) const;

	/** Where an output leads, in order. */
	TConstArrayView<FKzGraphLink> GetLinks(int32 Node, int32 Output) const;

	const FGuid& GetNodeGuid(int32 Node) const { return NodeGuids[Node]; }

	UPROPERTY()
	FInstancedStructContainer Nodes;

	UPROPERTY()
	TArray<FKzGraphNodeRanges> Ranges;

	UPROPERTY()
	TArray<FName> Inputs;

	UPROPERTY()
	TArray<FKzGraphOutput> Outputs;

	UPROPERTY()
	TArray<FKzGraphLink> Links;

	UPROPERTY()
	TArray<FKzGraphEntry> Entries;

	/** The editor node each node was compiled from: an id that survives recompiles, for runners that remember nodes (visits, saves). */
	UPROPERTY()
	TArray<FGuid> NodeGuids;
};

/** Base of graph assets. Each kind of graph inherits, tells the editor which nodes it takes and how they connect, and its runner reads the data. */
UCLASS(Abstract)
class KZGRAPH_API UKzGraph : public UObject
{
	GENERATED_BODY()

public:
	const FKzGraphData& GetData() const { return Data; }

#if WITH_EDITOR
	/** Base struct of the nodes this graph takes. */
	virtual const UScriptStruct* GetNodeStruct() const { return FKzGraphNode::StaticStruct(); }

	virtual EKzGraphFlow GetFlow() const { return EKzGraphFlow::Horizontal; }

	/** Zero for no limit. */
	virtual int32 GetMaxLinksPerOutput() const { return 0; }
	virtual int32 GetMaxLinksPerInput() const { return 0; }

	virtual bool CanConnect(FConstStructView From, FName Output, FConstStructView To, FName Input, FText& OutReason) const { return true; }
	virtual FLinearColor GetPinColor(FName Category) const { return FLinearColor::White; }

	/** Nodes a new graph starts with, such as its entry. */
	virtual void GetDefaultNodes(TArray<const UScriptStruct*>& OutNodes) const {}

	/** Checks of the graph as a whole, after each compile. */
	virtual void ValidateGraph(const FKzGraphData& InData, TArray<FKzValidationIssue>& OutIssues) const {}

	void SetData(FKzGraphData&& InData) { Data = MoveTemp(InData); }
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UEdGraph> EdGraph;
#endif

private:
	UPROPERTY()
	FKzGraphData Data;
};
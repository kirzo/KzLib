// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"
#include "EdGraph/EdGraph.h"
#include "KzEdGraph.generated.h"

class UKzGraph;

/** Editor graph of a graph asset: the source the asset's data is compiled from. */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** Creates the editor graph of an asset that has none, with the asset's default nodes. */
	static UKzEdGraph* CreateFor(UKzGraph& Graph);

	UKzGraph* GetGraphAsset() const;

	virtual void NotifyGraphChanged() override;
	virtual void NotifyGraphChanged(const FEdGraphEditAction& Action) override;
	virtual void PostEditUndo() override;

	/** Something that may change what the runtime reads was edited. */
	void MarkChanged() { bNeedsCompile = true; }
	bool NeedsCompile() const { return bNeedsCompile; }

	/** Compiles into the asset and shows the problems on their nodes. */
	void Compile();
	const TArray<FKzValidationIssue>& GetCompileIssues() const { return CompileIssues; }

	FSimpleMulticastDelegate OnCompiled;

	/** Announces an edit on the next tick, for wiring done from inside the graph panel, which still holds the pins. */
	void NotifyGraphChangedSoon();

	/** Recreates every node's pins from its struct; links follow by pin name. */
	void ReconstructNodes();

private:
	bool bNeedsCompile = false;
	bool bNotifyPending = false;
	TArray<FKzValidationIssue> CompileIssues;
};
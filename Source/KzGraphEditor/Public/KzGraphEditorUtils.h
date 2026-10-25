// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzGraphNode.h"

class UEdGraph;
class UEdGraphPin;
class UKzGraph;

namespace KzGraphEditor
{
	/** The graph asset an editor graph belongs to. */
	KZGRAPHEDITOR_API UKzGraph* GetGraphAsset(const UEdGraph* Graph);

	/** Sorts linked pins the way the graph orders links: by where their nodes sit across the flow. */
	KZGRAPHEDITOR_API void SortByFlow(TArray<UEdGraphPin*>& Pins, EKzGraphFlow Flow);

	/** The node inputs an output reaches through any reroutes, in link order. */
	KZGRAPHEDITOR_API void GatherTargets(const UEdGraphPin& Output, EKzGraphFlow Flow, TArray<UEdGraphPin*>& OutTargets);
}
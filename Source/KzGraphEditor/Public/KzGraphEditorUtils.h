// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzGraphNode.h"

class SWidget;
class UEdGraph;
class UEdGraphPin;
class UKzEdGraphNode;
class UKzGraph;

/** Builds the body a node shows under its title, such as the text of a dialogue line. */
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<SWidget>, FKzOnMakeGraphNodeBody, const UKzEdGraphNode&);

namespace KzGraphEditor
{
	/** Gives the nodes of Struct, and of the structs below it, a body under their title. Domains register from their editor module. */
	KZGRAPHEDITOR_API void RegisterNodeBody(const UScriptStruct* Struct, FKzOnMakeGraphNodeBody MakeBody);
	KZGRAPHEDITOR_API void UnregisterNodeBody(const UScriptStruct* Struct);

	/** The body registered for the node's struct or its closest parent; null for none. */
	TSharedPtr<SWidget> MakeNodeBody(const UKzEdGraphNode& Node);

	/** The graph asset an editor graph belongs to. */
	KZGRAPHEDITOR_API UKzGraph* GetGraphAsset(const UEdGraph* Graph);

	/** Sorts linked pins the way the graph orders links: by where their nodes sit across the flow. */
	KZGRAPHEDITOR_API void SortByFlow(TArray<UEdGraphPin*>& Pins, EKzGraphFlow Flow);

	/** The node inputs an output reaches through any reroutes, in link order. */
	KZGRAPHEDITOR_API void GatherTargets(const UEdGraphPin& Output, EKzGraphFlow Flow, TArray<UEdGraphPin*>& OutTargets);
}
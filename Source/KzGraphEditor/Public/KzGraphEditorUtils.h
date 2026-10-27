// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzGraphNode.h"

class FObjectProperty;
class SGraphEditor;
class SWidget;
class UEdGraph;
class UEdGraphPin;
class UKzEdGraphNode;
class UKzGraph;

/** Builds the body a node shows under its title, such as the text of a dialogue line. */
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<SWidget>, FKzOnMakeGraphNodeBody, const UKzEdGraphNode&);

/** Answers a double click on a node, such as opening what it references. */
DECLARE_DELEGATE_OneParam(FKzOnGraphNodeDoubleClicked, const UKzEdGraphNode&);

namespace KzGraphEditor
{
	/** Gives the nodes of Struct, and of the structs below it, a body under their title. Domains register from their editor module. */
	KZGRAPHEDITOR_API void RegisterNodeBody(const UScriptStruct* Struct, FKzOnMakeGraphNodeBody MakeBody);
	KZGRAPHEDITOR_API void UnregisterNodeBody(const UScriptStruct* Struct);

	/** The body registered for the node's struct or its closest parent; null for none. */
	TSharedPtr<SWidget> MakeNodeBody(const UKzEdGraphNode& Node);

	/** Gives the nodes of Struct, and of the structs below it, an answer to a double click. */
	KZGRAPHEDITOR_API void RegisterNodeDoubleClick(const UScriptStruct* Struct, FKzOnGraphNodeDoubleClicked OnDoubleClicked);
	KZGRAPHEDITOR_API void UnregisterNodeDoubleClick(const UScriptStruct* Struct);

	/** Runs the double click registered for the node's struct or its closest parent. */
	void DoubleClickNode(const UKzEdGraphNode& Node);

	/** Gives each object the struct holds, down instanced structs and arrays, a copy of its own in Outer when ShouldCopy picks it. */
	void DuplicateObjects(const UStruct* Struct, void* Memory, UObject& Outer, TFunctionRef<bool(const FObjectProperty&, const UObject&)> ShouldCopy);

	/** The graph asset an editor graph belongs to. */
	KZGRAPHEDITOR_API UKzGraph* GetGraphAsset(const UEdGraph* Graph);

	/** Sorts linked pins the way the graph orders links: by where their nodes sit across the flow. */
	KZGRAPHEDITOR_API void SortByFlow(TArray<UEdGraphPin*>& Pins, EKzGraphFlow Flow);

	/** The node inputs an output reaches through any reroutes, in link order. */
	KZGRAPHEDITOR_API void GatherTargets(const UEdGraphPin& Output, EKzGraphFlow Flow, TArray<UEdGraphPin*>& OutTargets);

	/** Lines the links between the selected nodes up along the graph's flow; the engine's own straightening only knows a horizontal one. */
	KZGRAPHEDITOR_API void StraightenConnections(SGraphEditor& Editor);

	/** Down a vertical flow, moves the nodes linked to Pin, or only the one OnlyTo is on, until their pins line up with it. */
	KZGRAPHEDITOR_API void StraightenPinConnections(SGraphEditor& Editor, UEdGraphPin& Pin, UEdGraphPin* OnlyTo = nullptr);
}
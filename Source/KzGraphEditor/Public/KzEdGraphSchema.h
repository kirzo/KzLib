// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "KzGraphNode.h"
#include "StructUtils/InstancedStruct.h"
#include "KzEdGraphSchema.generated.h"

class UKzEdGraphNode;
class UKzGraph;
struct FAssetData;
class UToolMenu;
class UGraphNodeContextMenuContext;
struct FKey;

/** Schema of every graph built on KzGraph. What differs between kinds of graph comes from their asset class. */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	/** Category of pins that declare none. */
	static const FName PC_Flow;

	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	/** Double-clicking a wire puts a reroute on it there. */
	virtual void OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2f& GraphPosition) const override;

	/** Assets dropped from the Content Browser make the nodes their graph asks for (UKzGraph::MakeNodeForAsset). */
	virtual void DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2f& GraphPosition, UEdGraph* Graph) const override;
	virtual void GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const override;

	virtual FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;

	/** Node structs a graph takes: children of its node struct, without Hidden metadata. */
	static void ForEachNodeStruct(const UKzGraph& Graph, TFunctionRef<void(const UScriptStruct*)> Callback);

	/** The node struct whose Shortcut metadata names the key, or null. */
	static const UScriptStruct* FindNodeStructForShortcut(const UKzGraph& Graph, const FKey& Key);

	/** Adds a node of the struct with its corner at Location. */
	static UKzEdGraphNode* CreateNode(UEdGraph& Graph, const UScriptStruct* NodeStruct, const FVector2f& Location, bool bSelect);

	/** Adds a node holding a copy of Node with its corner at Location. */
	static UKzEdGraphNode* CreateNode(UEdGraph& Graph, const TInstancedStruct<FKzGraphNode>& Node, const FVector2f& Location, bool bSelect);

private:
	/** Announces a wiring edit on the next tick. */
	static void NotifySoon(UEdGraph* Graph);
};

/** Context menu entry that adds a node of a struct. */
struct KZGRAPHEDITOR_API FKzGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	static FName StaticGetTypeId() { static FName Type("FKzGraphSchemaAction_NewNode"); return Type; }
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;

	const UScriptStruct* NodeStruct = nullptr;
};

/** Adds a comment box; wraps the selected nodes when there are any. */
struct KZGRAPHEDITOR_API FKzGraphSchemaAction_Comment : public FEdGraphSchemaAction
{
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;
};

/** Adds a reroute, wired to the pin dragged from. */
struct KZGRAPHEDITOR_API FKzGraphSchemaAction_Reroute : public FEdGraphSchemaAction
{
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;
};
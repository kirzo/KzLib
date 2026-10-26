// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "KzGraphNode.h"
#include "StructUtils/InstancedStruct.h"
#include "KzEdGraphNode.generated.h"

class UKzEdGraph;

/** Base of graph editor nodes: every change tells the graph, which then knows it needs a compile. */
UCLASS(Abstract)
class KZGRAPHEDITOR_API UKzEdGraphNodeBase : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** Moves count too: they reorder the links into the node. */
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostEditUndo() override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	/** Recreates the pins, carrying links over by pin name. */
	virtual void ReconstructNode() override;

protected:
	void NotifyGraphChanged() const;

	/** The graph needs a compile: what the runtime reads may have changed. */
	void MarkGraphChanged() const;
};

/** Holds one node struct of the graph and shows the pins, title and color it declares. */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraphNode : public UKzEdGraphNodeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Node", meta = (ShowOnlyInnerProperties, StructTypeConst))
	TInstancedStruct<FKzGraphNode> Node;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return Pin.PinFriendlyName; }
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;

	FText GetTitle() const;
};

/** Bends a wire and passes it on untouched, drawn as a dot. Compiles to nothing. */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraphNode_Reroute : public UKzEdGraphNodeBase
{
	GENERATED_BODY()

public:
	/** Adds one to the graph with its corner at Location. */
	static UKzEdGraphNode_Reroute* Create(UEdGraph& Graph, const FVector2f& Location, bool bSelect);

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return FText::GetEmpty(); }
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override;
	virtual UEdGraphPin* GetPassThroughPin(const UEdGraphPin* FromPin) const override;
};
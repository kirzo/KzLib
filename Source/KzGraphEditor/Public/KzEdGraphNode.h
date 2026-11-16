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

	/** A line under the title, which SKzGraphNode draws a grid cell high. */
	virtual FText GetSubtitle() const { return FText::GetEmpty(); }

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

	/** The title alone, in every form: SKzGraphNode draws the subtitle under it, a grid cell high, where the engine's multi-line titles miss the grid by a pixel. */
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

	/** The engine draws the title icon only for nodes that ask; a node without one shows nothing. */
	virtual bool ShowPaletteIconOnNode() const override { return true; }

	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return Pin.PinFriendlyName; }
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;

	FText GetTitle() const;
	virtual FText GetSubtitle() const override;
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

/**
 * Names a place Go To nodes lead to, with no wire across the graph: what its output links to takes the flow on. Its name is its title, renamed
 * with F2. Compiles to nothing; links go straight through it, as through a reroute.
 */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraphNode_Label : public UKzEdGraphNodeBase
{
	GENERATED_BODY()

public:
	UKzEdGraphNode_Label();

	UPROPERTY(EditAnywhere, Category = "Label")
	FName LabelName;

	/** The Labels of the graph with that name. */
	static TArray<UKzEdGraphNode_Label*> FindAll(const UEdGraph& Graph, FName Name);

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetSubtitle() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return FText::GetEmpty(); }

	/** Renaming it, on the node or in its details, renames the Go To nodes that lead to it. */
	virtual void OnRenameNode(const FString& NewName) override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;

private:
	/** Points the Go To nodes that led to OldName here, unless a Label or an Entry still answers to it. */
	void RetargetGoTos(FName OldName) const;

	FName NameBeforeEdit;
};

/**
 * Leads on to a Label, or to an Entry, picked on the node: as if wired to what its output links to. Double-clicking it goes there. A Label
 * and an Entry may not share a name.
 */
UCLASS()
class KZGRAPHEDITOR_API UKzEdGraphNode_GoTo : public UKzEdGraphNodeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Go To", meta = (GetOptions = "GetTargetNames"))
	FName Target;

	/** The Label of that name, else the Entry; none when there is neither, or more than one Label. */
	static UEdGraphNode* FindTarget(const UEdGraph& Graph, FName Name);
	UEdGraphNode* FindTarget() const;

	/** What a Go To can lead to: the graph's Labels, and its Entries, the nodes that name an entry. */
	static void GetTargets(const UEdGraph& Graph, TArray<UKzEdGraphNode_Label*>& OutLabels, TArray<UKzEdGraphNode*>& OutEntries);

	/** A target's name, a Label's or an Entry's. */
	static FName GetTargetName(const UEdGraphNode& Node);

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return FText::GetEmpty(); }

private:
	UFUNCTION()
	TArray<FString> GetTargetNames() const;
};
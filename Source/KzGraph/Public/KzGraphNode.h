// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"
#include "Textures/SlateIcon.h"
#include "KzGraphNode.generated.h"

class UKzGraph;

/** Which way a graph reads: where pins sit and how the links of one output are ordered. */
UENUM()
enum class EKzGraphFlow : uint8
{
	/** Inputs on the left, outputs on the right; the links of an output go top to bottom. */
	Horizontal,

	/** Inputs on top, outputs at the bottom; the links of an output go left to right. */
	Vertical
};

#if WITH_EDITOR
/** A pin as a node declares it to the editor. */
struct FKzGraphPinDesc
{
	FName Name;
	FText DisplayName;
	FName Category;
	FText ToolTip;
};
#endif

/** Base of every graph node. Graphs keep their nodes as these structs, compiled from the editor graph. */
USTRUCT()
struct KZGRAPH_API FKzGraphNode
{
	GENERATED_BODY()

	virtual ~FKzGraphNode() = default;

	/** Name a runner starts from when this node is an entry of the graph; none for any other node. */
	virtual FName GetEntryName() const { return NAME_None; }

#if WITH_EDITOR
	/** Pins can follow the graph's own settings, such as the inputs a combo declares. */
	virtual void GetInputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const;
	virtual void GetOutputPins(const UKzGraph& Graph, TArray<FKzGraphPinDesc>& OutPins) const;

	/** Empty takes the struct's display name. */
	virtual FText GetTitle() const { return FText::GetEmpty(); }
	virtual FText GetSubtitle() const { return FText::GetEmpty(); }
	virtual FLinearColor GetColor() const { return FLinearColor(0.15f, 0.15f, 0.15f); }
	virtual FSlateIcon GetIcon() const { return FSlateIcon(); }

	/** Extra texts the editor's search finds the node by. */
	virtual void GetSearchTexts(TArray<FText>& OutTexts) const {}

	virtual void Validate(TArray<FKzValidationIssue>& OutIssues) const {}

	/** After an edit in Details, before the pins are rebuilt: the place to keep ids unique. */
	virtual void PostEditChange() {}
	virtual bool CanUserDelete() const { return true; }
	virtual bool CanDuplicate() const { return true; }
#endif
};
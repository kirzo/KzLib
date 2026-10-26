// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KismetPins/SGraphPinExec.h"
#include "SGraphNode.h"

class SHorizontalBox;
class UKzEdGraphNode;

/** Flow pin colored by its graph's category colors. */
class SKzGraphPin : public SGraphPinExec
{
public:
	SLATE_BEGIN_ARGS(SKzGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:
	virtual FSlateColor GetPinColor() const override;
};

/** Node of a KzGraph: the engine's layout in horizontal graphs; pins above and below the title in vertical ones. */
class SKzGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SKzGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UKzEdGraphNode* InNode);

	virtual void UpdateGraphNode() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;

protected:
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;

private:
	/** The domain's body for this node, or an empty widget. */
	TSharedRef<SWidget> MakeBody() const;

	bool IsVertical() const;
	FSlateColor GetBodyColor() const;

	TSharedPtr<SHorizontalBox> TopPinBox;
	TSharedPtr<SHorizontalBox> BottomPinBox;
};
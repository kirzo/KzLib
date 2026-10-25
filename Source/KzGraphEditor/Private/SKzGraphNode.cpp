// Copyright 2026 kirzo

#include "SKzGraphNode.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

void SKzGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SGraphPinExec::Construct(SGraphPinExec::FArguments(), InPin);
}

FSlateColor SKzGraphPin::GetPinColor() const
{
	const UEdGraphPin* Pin = GetPinObj();
	const UKzGraph* Asset = Pin ? KzGraphEditor::GetGraphAsset(Pin->GetOwningNode()->GetGraph()) : nullptr;
	if (!Asset)
	{
		return SGraphPinExec::GetPinColor();
	}

	const FLinearColor Color = Asset->GetPinColor(Pin->PinType.PinCategory);
	return IsHovered() ? Color : Color * FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
}

void SKzGraphNode::Construct(const FArguments& InArgs, UKzEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SKzGraphNode::UpdateGraphNode()
{
	if (!IsVertical())
	{
		SGraphNode::UpdateGraphNode();
		return;
	}

	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	SetupErrorReporting();

	const TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(4.0f, 2.0f)
			[
				SAssignNew(TopPinBox, SHorizontalBox)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Graph.Node.ColorSpill"))
				.BorderBackgroundColor(this, &SKzGraphNode::GetBodyColor)
				.Padding(FMargin(10.0f, 4.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						CreateIconWidget()
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CreateTitleWidget(NodeTitle)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							NodeTitle.ToSharedRef()
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(4.0f, 2.0f)
			[
				SAssignNew(BottomPinBox, SHorizontalBox)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ErrorReporting->AsWidget()
			]
		]
	];

	CreatePinWidgets();
}

void SKzGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	if (!IsVertical())
	{
		SGraphNode::AddPin(PinToAdd);
		return;
	}

	PinToAdd->SetOwner(SharedThis(this));
	const bool bInput = PinToAdd->GetDirection() == EGPD_Input;
	(bInput ? TopPinBox : BottomPinBox)->AddSlot()
	.AutoWidth()
	.Padding(4.0f, 0.0f)
	[
		PinToAdd
	];
	(bInput ? InputPins : OutputPins).Add(PinToAdd);
}

TSharedPtr<SGraphPin> SKzGraphNode::CreatePinWidget(UEdGraphPin* Pin) const
{
	return SNew(SKzGraphPin, Pin);
}

bool SKzGraphNode::IsVertical() const
{
	const UKzGraph* Asset = GraphNode ? KzGraphEditor::GetGraphAsset(GraphNode->GetGraph()) : nullptr;
	return Asset && Asset->GetFlow() == EKzGraphFlow::Vertical;
}

FSlateColor SKzGraphNode::GetBodyColor() const
{
	return GraphNode ? GraphNode->GetNodeTitleColor() : FLinearColor::Gray;
}
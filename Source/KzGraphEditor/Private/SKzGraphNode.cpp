// Copyright 2026 kirzo

#include "SKzGraphNode.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

void SKzGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SGraphPinExec::Construct(SGraphPinExec::FArguments(), InPin);

	const UEdGraphNode* Node = InPin->GetOwningNode();
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(Node->GetGraph());
	if (!Asset || Asset->GetFlow() != EKzGraphFlow::Vertical)
	{
		return;
	}

	// Vertical flow: the arrow turned to point down along the flow, wires leaving below it and arriving above it.
	const TSharedRef<SWidget> Arrow = SNew(SImage)
		.Image_Lambda([this]() { return GetPinIcon(); })
		.ColorAndOpacity_Lambda([this]() { return GetPinColor(); })
		.RenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(90.0f))))
		.RenderTransformPivot(FVector2D(0.5f, 0.5f));
	Arrow->SetCursor(TAttribute<TOptional<EMouseCursor::Type>>::CreateLambda([this]() { return GetPinCursor(); }));
	SetPinImageWidget(Arrow);

	// A lone input or output needs no name; several are told apart by theirs.
	const int32 PinsOnSide = Node->Pins.FilterByPredicate([InPin](const UEdGraphPin* Pin) { return Pin->Direction == InPin->Direction; }).Num();
	TSharedRef<SWidget> Label = SNullWidget::NullWidget;
	if (PinsOnSide > 1)
	{
		Label = SNew(STextBlock)
			.Text_Lambda([this]() { return GetPinLabel(); })
			.TextStyle(FAppStyle::Get(), "Graph.Node.PinName")
			.ColorAndOpacity_Lambda([this]() { return GetPinTextColor(); });
	}

	const bool bInput = InPin->Direction == EGPD_Input;
	SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			bInput ? Arrow : Label
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			bInput ? Label : Arrow
		]);
}

FSlateColor SKzGraphPin::GetPinColor() const
{
	// A rebuilt node trashes its old pins, whose widgets still paint until the panel catches up.
	const UEdGraphPin* Pin = GetPinObj();
	const UEdGraphNode* Node = Pin && !Pin->IsPendingKill() ? Pin->GetOwningNodeUnchecked() : nullptr;
	const UKzGraph* Asset = Node ? Node->GetTypedOuter<UKzGraph>() : nullptr;
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

	// A big grid cell wide at least, so short titles do not make thin nodes.
	const float MinWidth = 8.0f * SNodePanel::GetSnapGridSize();
	static const FSlateRoundedBoxBrush TitleBrush(FLinearColor::White, 4.0f);

	// A side without pins takes no room, so the title band can be the top or the bottom of the node.
	const bool bHasInputs = GraphNode->Pins.ContainsByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Input; });
	const bool bHasOutputs = GraphNode->Pins.ContainsByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Output; });

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
		.Padding(0.0f)
		[
			SNew(SBox)
			.MinDesiredWidth(MinWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(bHasInputs ? FMargin(4.0f, 2.0f) : FMargin(0.0f))
				[
					SAssignNew(TopPinBox, SHorizontalBox)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2.0f)
				[
					SNew(SBorder)
					.BorderImage(&TitleBrush)
					.BorderBackgroundColor(this, &SKzGraphNode::GetBodyColor)
					.HAlign(HAlign_Center)
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
							.HAlign(HAlign_Center)
							[
								CreateTitleWidget(NodeTitle)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								NodeTitle.ToSharedRef()
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeBody()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(bHasOutputs ? FMargin(4.0f, 2.0f) : FMargin(0.0f))
				[
					SAssignNew(BottomPinBox, SHorizontalBox)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					ErrorReporting->AsWidget()
				]
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

void SKzGraphNode::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	MainBox->AddSlot()
	.AutoHeight()
	[
		MakeBody()
	];
}

TSharedRef<SWidget> SKzGraphNode::MakeBody() const
{
	const UKzEdGraphNode* Node = Cast<UKzEdGraphNode>(GraphNode);
	const TSharedPtr<SWidget> Body = Node ? KzGraphEditor::MakeNodeBody(*Node) : nullptr;
	return Body.IsValid() ? Body.ToSharedRef() : SNullWidget::NullWidget;
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
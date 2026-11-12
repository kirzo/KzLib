// Copyright 2026 kirzo

#include "SKzGraphFindResults.h"
#include "Algo/AllOf.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "KzEdGraphNode.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SKzGraphFindResults"

namespace
{
	// Everything a node can be found by, its title first.
	void GetSearchableTexts(const UEdGraphNode& Node, TArray<FText>& OutTexts)
	{
		OutTexts.Add(Node.GetNodeTitle(ENodeTitleType::FullTitle));
		if (!Node.NodeComment.IsEmpty())
		{
			OutTexts.Add(FText::FromString(Node.NodeComment));
		}

		const UKzEdGraphNode* KzNode = Cast<UKzEdGraphNode>(&Node);
		if (KzNode && KzNode->Node.IsValid())
		{
			if (const FText Subtitle = KzNode->GetSubtitle(); !Subtitle.IsEmpty())
			{
				OutTexts.Add(Subtitle);
			}
			KzNode->Node.Get().GetSearchTexts(OutTexts);
		}

		for (const UEdGraphPin* Pin : Node.Pins)
		{
			if (!Pin->PinFriendlyName.IsEmpty())
			{
				OutTexts.Add(Pin->PinFriendlyName);
			}
		}
	}
}

void SKzGraphFindResults::Construct(const FArguments& InArgs, UEdGraph* InGraph)
{
	Graph = InGraph;
	OnResultActivated = InArgs._OnResultActivated;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(4.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(LOCTEXT("SearchHint", "Search nodes, pins and comments"))
				.OnTextChanged(this, &SKzGraphFindResults::OnSearchChanged)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(List, SListView<FResultPtr>)
			.ListItemsSource(&Results)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SKzGraphFindResults::MakeRow)
			.OnMouseButtonClick(this, &SKzGraphFindResults::OnResultClicked)
		]
	];
}

void SKzGraphFindResults::FocusSearch()
{
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda([WeakBox = TWeakPtr<SSearchBox>(SearchBox)](double, float)
	{
		if (const TSharedPtr<SSearchBox> Box = WeakBox.Pin())
		{
			FSlateApplication::Get().SetKeyboardFocus(Box, EFocusCause::SetDirectly);
		}
		return EActiveTimerReturnType::Stop;
	}));
}

void SKzGraphFindResults::Refresh()
{
	Results.Reset();

	TArray<FString> Words;
	SearchText.ParseIntoArrayWS(Words);
	const UEdGraph* Target = Graph.Get();
	if (Target && !Words.IsEmpty())
	{
		auto Contains = [](const FText& Text, const FString& Word) { return Text.ToString().Contains(Word); };
		for (UEdGraphNode* Node : Target->Nodes)
		{
			TArray<FText> Texts;
			GetSearchableTexts(*Node, Texts);

			auto OnNode = [&Texts, &Contains](const FString& Word) { return Texts.ContainsByPredicate([&Word, &Contains](const FText& Text) { return Contains(Text, Word); }); };
			if (!Algo::AllOf(Words, OnNode))
			{
				continue;
			}

			const FResultPtr Result = MakeShared<FResult>();
			Result->Node = Node;
			Result->Title = Texts[0];

			// A match past the title shows next to it, so the row says why the node is there.
			if (!Contains(Texts[0], Words[0]))
			{
				Result->Match = *Texts.FindByPredicate([&Words, &Contains](const FText& Text) { return Contains(Text, Words[0]); });
			}
			Results.Add(Result);
		}
	}

	List->RequestListRefresh();
}

void SKzGraphFindResults::OnSearchChanged(const FText& Text)
{
	SearchText = Text.ToString();
	Refresh();
}

TSharedRef<ITableRow> SKzGraphFindResults::MakeRow(FResultPtr Result, const TSharedRef<STableViewBase>& Owner)
{
	const FText Highlight = FText::FromString(SearchText);
	return SNew(STableRow<FResultPtr>, Owner)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Result->Title).HighlightText(Highlight)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Result->Match).HighlightText(Highlight).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

void SKzGraphFindResults::OnResultClicked(FResultPtr Result)
{
	if (UEdGraphNode* Node = Result.IsValid() ? Result->Node.Get() : nullptr)
	{
		OnResultActivated.ExecuteIfBound(Node);
	}
}

#undef LOCTEXT_NAMESPACE
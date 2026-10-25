// Copyright 2026 kirzo

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ITableRow;
class SSearchBox;
class STableViewBase;
class UEdGraph;
class UEdGraphNode;

DECLARE_DELEGATE_OneParam(FOnKzGraphFindResultActivated, UEdGraphNode*);

/** Search over a graph: node titles, the texts nodes offer, pins and comments. Every word must appear on a node for it to match. */
class SKzGraphFindResults : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKzGraphFindResults) {}
		SLATE_EVENT(FOnKzGraphFindResultActivated, OnResultActivated)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraph* InGraph);

	/** Puts the keyboard in the search box, a frame from now so a tab just opened can take it. */
	void FocusSearch();

	/** Runs the search again, for after the graph changed. */
	void Refresh();

private:
	struct FResult
	{
		TWeakObjectPtr<UEdGraphNode> Node;
		FText Title;

		/** The text that matched, when the title did not. */
		FText Match;
	};
	using FResultPtr = TSharedPtr<FResult>;

	void OnSearchChanged(const FText& Text);
	TSharedRef<ITableRow> MakeRow(FResultPtr Result, const TSharedRef<STableViewBase>& Owner);
	void OnResultClicked(FResultPtr Result);

	TWeakObjectPtr<UEdGraph> Graph;
	FOnKzGraphFindResultActivated OnResultActivated;
	FString SearchText;
	TArray<FResultPtr> Results;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<FResultPtr>> List;
};
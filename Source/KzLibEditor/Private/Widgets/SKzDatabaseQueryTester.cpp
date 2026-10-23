// Copyright 2026 kirzo

#include "Widgets/SKzDatabaseQueryTester.h"
#include "Core/KzDatabaseAsset.h"

#include "IStructureDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/CoreStyle.h"
#include "UObject/StructOnScope.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SKzDatabaseQueryTester"

namespace KzDatabaseQueryTester
{
	const FName TableColumn(TEXT("Table"));
	const FName RowColumn(TEXT("Row"));
	const FName TagsColumn(TEXT("Tags"));
	const FName ResultColumn(TEXT("Result"));

	FText RowLabel(const SKzDatabaseQueryTester::FResult& Result)
	{
		return Result.ID.IsNone() ? FText::AsNumber(Result.Index + 1) : FText::Format(LOCTEXT("RowWithId", "{0} ({1})"), FText::AsNumber(Result.Index + 1), FText::FromName(Result.ID));
	}

	FText Describe(const SKzDatabaseQueryTester::FResult& Result)
	{
		switch (Result.Match)
		{
		case EKzDatabaseMatch::Candidate:
			return FText::Format(Result.bWinner ? LOCTEXT("Wins", "Wins, score {0}") : LOCTEXT("Candidate", "Candidate, score {0}"), Result.Score);
		case EKzDatabaseMatch::MissingRequiredTag:
			return LOCTEXT("MissingRequired", "Lacks a required tag");
		case EKzDatabaseMatch::HasIgnoredTag:
			return LOCTEXT("HasIgnored", "Has an ignored tag");
		case EKzDatabaseMatch::FailsTagQuery:
			return LOCTEXT("FailsTagQuery", "Fails the tag query");
		case EKzDatabaseMatch::UncoveredTag:
			return FText::Format(LOCTEXT("Uncovered", "{0} is neither asked for nor in the context"), FText::FromName(Result.UncoveredTag.GetTagName()));
		}
		return FText::GetEmpty();
	}

	class SResultRow : public SMultiColumnTableRow<TSharedPtr<SKzDatabaseQueryTester::FResult>>
	{
	public:
		SLATE_BEGIN_ARGS(SResultRow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, const TSharedPtr<SKzDatabaseQueryTester::FResult>& InResult)
		{
			Result = InResult;
			FSuperRowType::Construct(FSuperRowType::FArguments().Padding(FMargin(0.0f, 2.0f)), OwnerTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
		{
			const FText Text = Column == TableColumn ? FText::FromString(GetNameSafe(Result->Table.Get()))
				: Column == RowColumn ? RowLabel(*Result)
				: Column == TagsColumn ? FText::FromString(Result->Tags)
				: Describe(*Result);

			// Rejected rows are dimmed, so the winner and the candidates it beat stand out.
			return SNew(STextBlock)
				.Text(Text)
				.Font(FCoreStyle::GetDefaultFontStyle(Result->bWinner ? "Bold" : "Regular", 9))
				.ColorAndOpacity(Result->Match == EKzDatabaseMatch::Candidate ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground());
		}

	private:
		TSharedPtr<SKzDatabaseQueryTester::FResult> Result;
	};
}

void SKzDatabaseQueryTester::Construct(const FArguments& InArgs, UKzDatabaseAsset* InAsset)
{
	Asset = InAsset;
	Query = MakeShared<FStructOnScope>(FKzDatabaseQuery::StaticStruct());

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = false;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	QueryView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateStructureDetailView(ViewArgs, FStructureDetailsViewArgs(), Query);
	QueryView->GetOnFinishedChangingPropertiesDelegate().AddSPLambda(this, [this](const FPropertyChangedEvent&) { Refresh(); });

	PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SKzDatabaseQueryTester::HandleObjectPropertyChanged);

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.Value(0.4f)
		[
			QueryView->GetWidget().ToSharedRef()
		]
		+ SSplitter::Slot()
		.Value(0.6f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 6.0f))
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(this, &SKzDatabaseQueryTester::GetSummary)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(ListView, SListView<TSharedPtr<FResult>>)
				.ListItemsSource(&Results)
				.OnGenerateRow(this, &SKzDatabaseQueryTester::OnGenerateRow)
				.SelectionMode(ESelectionMode::None)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(KzDatabaseQueryTester::TableColumn).DefaultLabel(LOCTEXT("TableColumn", "Table")).FillWidth(0.2f)
					+ SHeaderRow::Column(KzDatabaseQueryTester::RowColumn).DefaultLabel(LOCTEXT("RowColumn", "Row")).FillWidth(0.15f)
					+ SHeaderRow::Column(KzDatabaseQueryTester::TagsColumn).DefaultLabel(LOCTEXT("TagsColumn", "Tags")).FillWidth(0.35f)
					+ SHeaderRow::Column(KzDatabaseQueryTester::ResultColumn).DefaultLabel(LOCTEXT("ResultColumn", "Result")).FillWidth(0.3f)
				)
			]
		]
	];

	Refresh();
}

SKzDatabaseQueryTester::~SKzDatabaseQueryTester()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
}

void SKzDatabaseQueryTester::Refresh()
{
	Results.Reset();

	const FKzDatabaseQuery& CurrentQuery = *reinterpret_cast<const FKzDatabaseQuery*>(Query->GetStructMemory());
	const UKzDatabaseAsset* Root = Asset.Get();
	const FKzDatabaseItem* Winner = Root ? Root->ResolveMatch(CurrentQuery) : nullptr;

	// The table, then each parent, in the order a query walks them; a loop is walked once.
	TSet<const UKzDatabaseAsset*> Visited;
	for (const UKzDatabaseAsset* Table = Root; Table && !Visited.Contains(Table); Table = Table->ParentDatabase)
	{
		Visited.Add(Table);
		for (int32 Index = 0; Index < Table->Database.Items.Num(); ++Index)
		{
			const FKzDatabaseItem& Item = Table->Database.Items[Index];
			const TSharedPtr<FResult> Result = MakeShared<FResult>();
			Result->Table = Table;
			Result->Index = Index;
			Result->ID = Item.ID;
			Result->Tags = Item.Tags.ToStringSimple();
			Result->Match = CurrentQuery.Evaluate(Item.Tags, &Result->UncoveredTag);
			Result->Score = CurrentQuery.CalculateScore(Item.Tags);
			Result->bWinner = &Item == Winner;
			Results.Add(Result);
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SKzDatabaseQueryTester::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// Any table may be a parent of this one; edits are rare enough to just refresh.
	if (Cast<UKzDatabaseAsset>(Object))
	{
		Refresh();
	}
}

TSharedRef<ITableRow> SKzDatabaseQueryTester::OnGenerateRow(TSharedPtr<FResult> Result, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(KzDatabaseQueryTester::SResultRow, OwnerTable, Result);
}

FText SKzDatabaseQueryTester::GetSummary() const
{
	const TSharedPtr<FResult>* Winner = Results.FindByPredicate([](const TSharedPtr<FResult>& Result) { return Result->bWinner; });
	if (!Winner)
	{
		return LOCTEXT("NoWinner", "No row matches the query.");
	}
	return FText::Format(LOCTEXT("Winner", "Row {0} of {1} wins with score {2}."), KzDatabaseQueryTester::RowLabel(**Winner), FText::FromString(GetNameSafe((*Winner)->Table.Get())), (*Winner)->Score);
}

#undef LOCTEXT_NAMESPACE
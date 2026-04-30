// Copyright 2026 kirzo

#include "Widgets/SKzClassCombo.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SComboButton.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "UObject/UObjectIterator.h"
#include "SPositiveActionButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

void SKzClassCombo::Construct(const FArguments& InArgs)
{
	BaseClass = InArgs._BaseClass ? InArgs._BaseClass : UObject::StaticClass();
	OnClassSelectedDelegate = InArgs._OnClassSelected;
	OnGetDisallowedClassesDelegate = InArgs._OnGetDisallowedClasses;
	ClassFilterOptions = InArgs._ClassFilterOptions;

	FText ItemName = InArgs._ItemName.IsEmpty() ? INVTEXT("Element") : InArgs._ItemName;
	FText FinalButtonText = InArgs._ButtonText.IsEmpty() ? FText::Format(INVTEXT("Add {0}"), ItemName) : InArgs._ButtonText;
	FText FinalButtonTooltip = InArgs._ButtonTooltip.IsEmpty() ? FText::Format(INVTEXT("Add a new {0}."), ItemName) : InArgs._ButtonTooltip;

	TextFilter = MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString);

	SAssignNew(SearchBox, SSearchBox)
		.HintText(FText::Format(INVTEXT("Search {0}s"), ItemName))
		.OnTextChanged(this, &SKzClassCombo::OnSearchBoxTextChanged);

	SAssignNew(ListViewWidget, SListView<FKzClassComboEntryPtr>)
		.ListItemsSource(&FilteredEntries)
		.OnSelectionChanged(this, &SKzClassCombo::OnSelectionChanged)
		.OnGenerateRow(this, &SKzClassCombo::GenerateRow)
		.SelectionMode(ESelectionMode::Single);

	ChildSlot
		[
			SAssignNew(ComboButton, SPositiveActionButton)
				.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
				.Text(FinalButtonText)
				.ToolTipText(FinalButtonTooltip)
				.OnGetMenuContent(this, &SKzClassCombo::GetMenuContent)
				.OnComboBoxOpened(this, &SKzClassCombo::OnMenuOpened)
		];

	ComboButton->SetMenuContentWidgetToFocus(SearchBox);
}

TSharedRef<SWidget> SKzClassCombo::GetMenuContent()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(2.0f)
		[
			SNew(SBox)
				.WidthOverride(250.0f)
				[
					SNew(SVerticalBox)

						// Search and Filter Bar
						+ SVerticalBox::Slot()
						.Padding(1.f)
						.AutoHeight()
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								[
									SearchBox.ToSharedRef()
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(2.0f, 2.0f)
								[
									SNew(SComboButton)
										.ContentPadding(0.0f)
										.ForegroundColor(FSlateColor::UseForeground())
										.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
										.HasDownArrow(false)
										.Visibility(this, &SKzClassCombo::GetFilterOptionsButtonVisibility)
										.OnGetMenuContent(this, &SKzClassCombo::GetFilterOptionsMenuContent)
										.ButtonContent()
										[
											SNew(SImage)
												.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
												.ColorAndOpacity(FSlateColor::UseForeground())
										]
								]
						]

					// Class List View
					+ SVerticalBox::Slot()
						.MaxHeight(400.0f)
						[
							ListViewWidget.ToSharedRef()
						]
				]
		];
}

EVisibility SKzClassCombo::GetFilterOptionsButtonVisibility() const
{
	return ClassFilterOptions.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SKzClassCombo::GetFilterOptionsMenuContent()
{
	const bool bCloseSelfOnly = true;
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly);

	if (ClassFilterOptions.Num() > 0)
	{
		MenuBuilder.BeginSection("ClassFilterOptions", INVTEXT("Class Filters"));
		{
			for (const TSharedRef<FClassViewerFilterOption>& ClassFilterOption : ClassFilterOptions)
			{
				MenuBuilder.AddMenuEntry(
					ClassFilterOption->LabelText,
					ClassFilterOption->ToolTipText,
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &SKzClassCombo::ToggleFilterOption, ClassFilterOption),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &SKzClassCombo::IsFilterOptionEnabled, ClassFilterOption)
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
				);
			}
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SKzClassCombo::ToggleFilterOption(TSharedRef<FClassViewerFilterOption> FilterOption)
{
	FilterOption->bEnabled = !FilterOption->bEnabled;

	if (FilterOption->OnOptionChanged.IsBound())
	{
		FilterOption->OnOptionChanged.Execute(FilterOption->bEnabled);
	}

	UpdateClassList();
}

bool SKzClassCombo::IsFilterOptionEnabled(TSharedRef<FClassViewerFilterOption> FilterOption) const
{
	return FilterOption->bEnabled;
}

void SKzClassCombo::OnMenuOpened()
{
	SearchBox->SetText(FText::GetEmpty());
	TextFilter->SetFilterText(FText::GetEmpty());

	UpdateClassList();

	if (FilteredEntries.Num() > 0)
	{
		ListViewWidget->RequestScrollIntoView(FilteredEntries[0]);
	}
}

void SKzClassCombo::UpdateClassList()
{
	AllEntries.Empty();

	TSet<UClass*> DisallowedClasses;
	if (OnGetDisallowedClassesDelegate.IsBound())
	{
		DisallowedClasses = OnGetDisallowedClassesDelegate.Execute();
	}

	TMap<FString, TArray<UClass*>> CategorizedClasses;

	// Iterate over all UClasses to find valid children of our BaseClass
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;

		if (Class->IsChildOf(BaseClass) &&
			!Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) &&
			!Class->GetName().StartsWith(TEXT("SKEL_")) &&
			!Class->GetName().StartsWith(TEXT("REINST_")))
		{
			if (!DisallowedClasses.Contains(Class))
			{
				FString Category = Class->GetMetaData(TEXT("Category"));
				if (Category.IsEmpty())
				{
					Category = Class->GetMetaData(TEXT("ClassGroupNames"));
				}
				if (Category.IsEmpty())
				{
					Category = TEXT("Uncategorized");
				}

				CategorizedClasses.FindOrAdd(Category).Add(Class);
			}
		}
	}

	// Sort categories alphabetically
	CategorizedClasses.KeySort([](const FString& A, const FString& B) { return A < B; });

	bool bIsFirstCategory = true;

	for (auto& CatPair : CategorizedClasses)
	{
		TArray<UClass*>& Classes = CatPair.Value;

		// Sort classes within the category
		Classes.Sort([](const UClass& A, const UClass& B) {
			return A.GetDisplayNameText().ToString() < B.GetDisplayNameText().ToString();
			});

		// Add spacing between categories (except for the first one)
		if (!bIsFirstCategory)
		{
			AllEntries.Add(MakeShared<FKzClassComboEntry>()); // Separator
		}
		bIsFirstCategory = false;

		AllEntries.Add(MakeShared<FKzClassComboEntry>(CatPair.Key)); // Heading

		for (UClass* Class : Classes)
		{
			AllEntries.Add(MakeShared<FKzClassComboEntry>(Class));
		}
	}

	GenerateFilteredList();
}

void SKzClassCombo::GenerateFilteredList()
{
	FilteredEntries.Empty();

	FKzClassComboEntryPtr PendingHeading;
	FKzClassComboEntryPtr PendingSeparator;

	const bool bHasFilterText = !TextFilter->GetFilterText().IsEmpty();

	for (const FKzClassComboEntryPtr& Entry : AllEntries)
	{
		if (Entry->IsSeparator())
		{
			PendingSeparator = Entry;
		}
		else if (Entry->IsHeading())
		{
			PendingHeading = Entry;
		}
		else if (Entry->IsClass())
		{
			bool bPassesFilter = true;
			if (bHasFilterText)
			{
				const FString ClassName = Entry->Class->GetDisplayNameText().ToString();
				bPassesFilter = TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(ClassName));
			}

			if (bPassesFilter)
			{
				// Push any pending separators and headings before adding the valid class
				if (PendingSeparator.IsValid() && FilteredEntries.Num() > 0)
				{
					FilteredEntries.Add(PendingSeparator);
					PendingSeparator.Reset();
				}

				if (PendingHeading.IsValid())
				{
					FilteredEntries.Add(PendingHeading);
					PendingHeading.Reset();
				}

				FilteredEntries.Add(Entry);
			}
		}
	}

	if (ListViewWidget.IsValid())
	{
		ListViewWidget->RequestListRefresh();
	}
}

void SKzClassCombo::OnSearchBoxTextChanged(const FText& InSearchText)
{
	TextFilter->SetFilterText(InSearchText);
	SearchBox->SetError(TextFilter->GetFilterErrorText());
	GenerateFilteredList();
}

TSharedRef<ITableRow> SKzClassCombo::GenerateRow(FKzClassComboEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const
{
	check(Entry->IsHeading() || Entry->IsSeparator() || Entry->IsClass());

	if (Entry->IsHeading())
	{
		return SNew(STableRow<FKzClassComboEntryPtr>, OwnerTable)
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.NoHoverTableRow"))
			.ShowSelection(false)
			[
				SNew(SBox)
					.Padding(1.f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Entry->HeadingText))
							.TextStyle(FAppStyle::Get(), "Menu.Heading")
					]
			];
	}

	if (Entry->IsSeparator())
	{
		return SNew(STableRow<FKzClassComboEntryPtr>, OwnerTable)
			.Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.NoHoverTableRow"))
			.ShowSelection(false)
			[
				SNew(SSeparator)
					.SeparatorImage(FAppStyle::Get().GetBrush("Menu.Separator"))
					.Thickness(1.0f)
			];
	}

	// It's a standard class entry
	return SNew(STableRow<FKzClassComboEntryPtr>, OwnerTable)
		.ToolTipText(Entry->Class->GetToolTipText())
		[
			SNew(SHorizontalBox)

				// Left indent
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SSpacer)
						.Size(FVector2D(8.0f, 1.0f))
				]

				// Class Icon
				+ SHorizontalBox::Slot()
				.Padding(1.0f)
				.AutoWidth()
				[
					SNew(SImage)
						.Image(FSlateIconFinder::FindIconBrushForClass(Entry->Class))
				]

				// Space between icon and text
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SSpacer)
						.Size(FVector2D(3.0f, 1.0f))
				]

				// Class Name
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.HighlightText(this, &SKzClassCombo::GetSearchText)
						.Text(Entry->Class->GetDisplayNameText())
				]
		];
}

FText SKzClassCombo::GetSearchText() const
{
	return TextFilter->GetFilterText();
}

void SKzClassCombo::OnSelectionChanged(FKzClassComboEntryPtr SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid() && SelectedItem->IsClass())
	{
		ListViewWidget->ClearSelection();
		FSlateApplication::Get().DismissAllMenus();
		OnClassSelectedDelegate.ExecuteIfBound(SelectedItem->Class);
	}
}
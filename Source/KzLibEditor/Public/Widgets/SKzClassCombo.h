// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "ClassViewerFilter.h"

// Forward declarations
class FTextFilterExpressionEvaluator;
class SSearchBox;
class SPositiveActionButton;

// Internal structure to represent items in the combo list (Classes, Headings, or Separators)
struct FKzClassComboEntry
{
	UClass* Class = nullptr;
	FString HeadingText;

	bool IsHeading() const { return Class == nullptr && !HeadingText.IsEmpty(); }
	bool IsClass() const { return Class != nullptr; }
	bool IsSeparator() const { return Class == nullptr && HeadingText.IsEmpty(); }

	FKzClassComboEntry(UClass* InClass) : Class(InClass) {}
	FKzClassComboEntry(const FString& InHeadingText) : Class(nullptr), HeadingText(InHeadingText) {}
	FKzClassComboEntry() : Class(nullptr), HeadingText(TEXT("")) {}
};

typedef TSharedPtr<FKzClassComboEntry> FKzClassComboEntryPtr;

/**
 * A generic combo box widget for selecting UClass types.
 */
class KZLIBEDITOR_API SKzClassCombo : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnClassSelected, UClass* /*SelectedClass*/);
	DECLARE_DELEGATE_RetVal(TSet<UClass*>, FGetDisallowedClasses);

	SLATE_BEGIN_ARGS(SKzClassCombo)
		: _BaseClass(UObject::StaticClass())
		{
		}
		// The base class from which to filter available children classes
		SLATE_ARGUMENT(UClass*, BaseClass)
		SLATE_ARGUMENT(FText, ButtonText)
		SLATE_ARGUMENT(FText, ButtonTooltip)
		SLATE_ARGUMENT(FText, ItemName)
		SLATE_ARGUMENT(TArray<TSharedRef<FClassViewerFilterOption>>, ClassFilterOptions)
		SLATE_EVENT(FGetDisallowedClasses, OnGetDisallowedClasses)
		SLATE_EVENT(FOnClassSelected, OnClassSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// Slate Data
	UClass* BaseClass = nullptr;
	FOnClassSelected OnClassSelectedDelegate;
	FGetDisallowedClasses OnGetDisallowedClassesDelegate;
	TArray<TSharedRef<FClassViewerFilterOption>> ClassFilterOptions;

	// Widget Pointers
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<FKzClassComboEntryPtr>> ListViewWidget;
	TSharedPtr<SPositiveActionButton> ComboButton;

	// List Data
	TArray<FKzClassComboEntryPtr> AllEntries;
	TArray<FKzClassComboEntryPtr> FilteredEntries;

	// UI Generation
	TSharedRef<SWidget> GetMenuContent();
	TSharedRef<SWidget> GetFilterOptionsMenuContent();
	EVisibility GetFilterOptionsButtonVisibility() const;
	TSharedRef<ITableRow> GenerateRow(FKzClassComboEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const;

	// Filtering & Updates
	void OnMenuOpened();
	void UpdateClassList();
	void GenerateFilteredList();
	void OnSearchBoxTextChanged(const FText& InSearchText);
	FText GetSearchText() const;

	// Callbacks
	void ToggleFilterOption(TSharedRef<FClassViewerFilterOption> FilterOption);
	bool IsFilterOptionEnabled(TSharedRef<FClassViewerFilterOption> FilterOption) const;
	void OnSelectionChanged(FKzClassComboEntryPtr SelectedItem, ESelectInfo::Type SelectInfo);
};
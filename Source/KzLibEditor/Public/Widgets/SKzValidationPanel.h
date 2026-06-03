// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Core/KzValidationTypes.h"

class SSearchBox;
class SComboButton;

/**
 * Reusable Slate panel that displays a list of FKzValidationIssue entries with:
 *  - Severity icon and colored label
 *  - Issue message and validator id badge
 *  - Click-to-jump via the OnIssueActivated delegate
 *  - A filter button to toggle severities
 *  - A run/refresh button at the top
 */
class KZLIBEDITOR_API SKzValidationPanel : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnIssueActivated, const FKzValidationIssue& /*Issue*/);
	DECLARE_DELEGATE_RetVal(TArray<FKzValidationIssue>, FOnRunValidation);

	SLATE_BEGIN_ARGS(SKzValidationPanel) {}
		/** Called when an issue row is clicked. Editor implements jump-to behaviour. */
		SLATE_EVENT(FOnIssueActivated, OnIssueActivated)
		/** Called when the Validate button is clicked. Editor returns the fresh issue list. */
		SLATE_EVENT(FOnRunValidation, OnRunValidation)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Programmatically refresh by running OnRunValidation again. */
	void RefreshIssues();

	/** Replace the issue list directly without invoking OnRunValidation. */
	void SetIssues(const TArray<FKzValidationIssue>& InIssues);

private:
	using FIssuePtr = TSharedPtr<FKzValidationIssue>;

	TSharedRef<ITableRow> OnGenerateRow(FIssuePtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnRowClicked(FIssuePtr Item);
	TSharedRef<SWidget> BuildSeverityFilterMenu();
	TSharedRef<SWidget> BuildValidatorFilterMenu();
	void RebuildVisibleIssues();
	int32 CountActiveFilters() const;

	/** Sorted unique ValidatorIds across AllIssues. Cached when SetIssues runs so menus and tooltips reuse it. */
	TArray<FName> CollectValidatorIds() const;

	const FSlateBrush* GetSeverityBrush(EKzValidationSeverity Severity) const;
	FSlateColor        GetSeverityColor(EKzValidationSeverity Severity) const;
	FText              GetSeverityLabel(EKzValidationSeverity Severity) const;

	FOnIssueActivated OnIssueActivatedDelegate;
	FOnRunValidation  OnRunValidationDelegate;

	TArray<FIssuePtr> AllIssues;
	TArray<FIssuePtr> VisibleIssues;

	TSet<EKzValidationSeverity> EnabledSeverities;

	/** Validator IDs the user has explicitly hidden. Default-empty means "show all", including any newly-seen validator. */
	TSet<FName> DisabledValidatorIds;

	TSharedPtr<SListView<FIssuePtr>> ListView;
	TSharedPtr<SComboButton>		 FilterButton;
	TSharedPtr<SComboButton>		 ValidatorFilterButton;
};
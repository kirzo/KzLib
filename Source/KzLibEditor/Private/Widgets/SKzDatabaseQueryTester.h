// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzDatabase.h"
#include "EditorUndoClient.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FStructOnScope;
class IStructureDetailsView;
class UKzDatabaseAsset;

/** Tries a query on a database table and its parents: which row wins, and why each other row loses. */
class SKzDatabaseQueryTester : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:
	/** A row of the table or of a parent, and how the query treats it. */
	struct FResult
	{
		TWeakObjectPtr<const UKzDatabaseAsset> Table;
		int32 Index = INDEX_NONE;
		FName ID;
		FString Tags;
		EKzDatabaseMatch Match = EKzDatabaseMatch::Candidate;
		FGameplayTag UncoveredTag;
		int32 Score = 0;
		bool bWinner = false;
	};

	SLATE_BEGIN_ARGS(SKzDatabaseQueryTester) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UKzDatabaseAsset* InAsset);
	virtual ~SKzDatabaseQueryTester() override;

	virtual void PostUndo(bool bSuccess) override { Refresh(); }
	virtual void PostRedo(bool bSuccess) override { Refresh(); }

private:
	void Refresh();
	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FResult> Result, const TSharedRef<STableViewBase>& OwnerTable);
	FText GetSummary() const;

	TWeakObjectPtr<UKzDatabaseAsset> Asset;
	TSharedPtr<FStructOnScope> Query;
	TSharedPtr<IStructureDetailsView> QueryView;
	TArray<TSharedPtr<FResult>> Results;
	TSharedPtr<SListView<TSharedPtr<FResult>>> ListView;
	FDelegateHandle PropertyChangedHandle;
};
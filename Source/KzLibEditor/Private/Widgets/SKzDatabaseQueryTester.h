// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzDatabaseSet.h"
#include "EditorUndoClient.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FStructOnScope;
class IStructureDetailsView;

/**
 * Tries a query about an object made by a user: which row wins and whose it is, and why every other row loses. The object and the user are each a
 * database table, a Blueprint with a database component, or an asset holding one, such as an item with a Database fragment.
 */
class SKzDatabaseQueryTester : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:
	static const FName TabId;

	/** A row of either side's tables, and how the query treats it. */
	struct FResult
	{
		bool bUser = false;
		FString Table;
		int32 Index = INDEX_NONE;
		FName ID;
		FString Tags;
		EKzDatabaseMatch Match = EKzDatabaseMatch::Candidate;
		FGameplayTag UncoveredTag;
		int32 Score = 0;
		bool bWinner = false;
	};

	SLATE_BEGIN_ARGS(SKzDatabaseQueryTester)
		: _Object(nullptr)
		{}
		/** The object to fix, such as the table an asset editor shows; without one, it is picked. */
		SLATE_ARGUMENT(UObject*, Object)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SKzDatabaseQueryTester() override;

	virtual void PostUndo(bool bSuccess) override { Refresh(); }
	virtual void PostRedo(bool bSuccess) override { Refresh(); }

private:
	/** One side of the query, as picked: a table asset, or an asset holding a database owner. */
	struct FSide
	{
		TWeakObjectPtr<UObject> Source;

		/** Wraps a picked table asset, to read it as a set. */
		FKzDatabaseSet AssetTables;
	};

	/** The side's tables and what it is, found again on each refresh, since Blueprint compiles replace component templates. */
	const FKzDatabaseSet* GetTables(FSide& Side, FGameplayTagContainer& OutTags);

	void Refresh();
	void AddRows(const FKzDatabaseSet& Tables, bool bUser, const FKzDatabaseQuery& ContextQuery, const FKzDatabaseItem* Winner);
	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	TSharedRef<SWidget> MakeSourcePicker(FSide& Side);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FResult> Result, const TSharedRef<STableViewBase>& OwnerTable);

	FSide ObjectSide;
	FSide UserSide;

	/** The type of value asked for; the first table's until one is picked. */
	FKzTypeDef Type;

	TSharedPtr<FStructOnScope> Query;
	TSharedPtr<IStructureDetailsView> QueryView;
	TArray<TSharedPtr<FResult>> Results;
	TSharedPtr<SListView<TSharedPtr<FResult>>> ListView;
	FText Summary;
	FDelegateHandle PropertyChangedHandle;
};
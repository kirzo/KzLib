// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "EditorUndoClient.h"
#include "Input/DragAndDrop.h"

class IPropertyHandle;
class IPropertyHandleArray;
class FTextFilterExpressionEvaluator;
class SSearchBox;
class FUICommandList;
class FKzPropertyStackRowCustomizer;
class FStructOnScope;

/**
 * A single row in the stack. Wraps either an editable array element (Handle set) or an
 * injected read-only row from an external source (Snapshot set, bEditable = false), such
 * as a parent asset's inherited entries.
 */
struct KZLIBEDITOR_API FKzStackRow
{
	/** The editable array element handle. Null for injected/immutable rows. */
	TSharedPtr<IPropertyHandle> Handle;

	/** Read-only struct snapshot shown in the details panel for immutable rows. */
	TSharedPtr<FStructOnScope> Snapshot;

	/** Display label for immutable rows. Editable rows resolve their label from Handle. */
	FText DisplayLabel;

	/** Optional origin tag shown on the row (any row, editable or not). */
	FText SourceTag;

	/** Group this row lives under. Empty = ungrouped (rendered flat, before groups). */
	FText GroupName;

	/** Editable rows can be deleted/dragged/edited; immutable rows cannot. */
	bool bEditable = true;

	/** Immutable row whose key is overridden by an editable row (rendered dimmed). */
	bool bIsOverridden = false;

	/** Transient: set during filtering when this row is the first visible one in its group. */
	bool bRenderGroupHeader = false;

	FKzStackRow() = default;
	explicit FKzStackRow(TSharedPtr<IPropertyHandle> InHandle) : Handle(InHandle) {}

	/** True when the row is a real, mutable array element. */
	bool IsEditable() const { return bEditable && Handle.IsValid(); }
};

/** Provides injected read-only rows (e.g. inherited parent entries). Rebuilt on every refresh. */
DECLARE_DELEGATE_RetVal(TArray<TSharedPtr<FKzStackRow>>, FKzImmutableRowsProvider);

/** Drag&drop payload supporting multiple property handles. */
class FKzPropertyDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FKzPropertyDragDropOp, FDragDropOperation)

		TArray<TSharedPtr<IPropertyHandle>> HandlesToDrag;

	static TSharedRef<FKzPropertyDragDropOp> New(const TArray<TSharedPtr<IPropertyHandle>>& InHandlesToDrag)
	{
		TSharedRef<FKzPropertyDragDropOp> Operation = MakeShared<FKzPropertyDragDropOp>();
		Operation->HandlesToDrag = InHandlesToDrag;
		Operation->Construct();
		return Operation;
	}
};

class KZLIBEDITOR_API SKzPropertyStack : public SCompoundWidget, public FEditorUndoClient
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, const TArray<TSharedPtr<IPropertyHandle>>& /*SelectedHandles*/);
	DECLARE_DELEGATE_OneParam(FOnImmutableRowSelected, TSharedPtr<FKzStackRow> /*Row*/);

	SLATE_BEGIN_ARGS(SKzPropertyStack)
		: _bAllowDuplicates(false)
		, _ListPadding(FMargin(5.0f))
		{
		}
		SLATE_ARGUMENT(bool, bAllowDuplicates)
		SLATE_ARGUMENT(FText, ItemName)
		SLATE_ARGUMENT(FText, ItemNamePlural)
		SLATE_ARGUMENT(TSharedPtr<FKzPropertyStackRowCustomizer>, RowCustomizer)
		SLATE_ARGUMENT(FMargin, ListPadding)
		/** Fired when the selection changes. The array contains all currently selected
		 *  EDITABLE handles, in the order the list view returns them (top-to-bottom). */
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
		/** Fired when the primary selected row is an immutable (read-only) injected row. */
		SLATE_EVENT(FOnImmutableRowSelected, OnImmutableRowSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle);
	virtual ~SKzPropertyStack();

	void SetPropertyHandle(TSharedPtr<IPropertyHandle> InPropertyHandle);

	/** Sets (or clears) the provider of injected read-only rows and refreshes. */
	void SetImmutableRowsProvider(FKzImmutableRowsProvider InProvider);

	/** Rebuilds all rows (e.g. after the asset's inheritance source changed). */
	void RefreshRows();

	/** Returns all selected EDITABLE handles in their current list order. */
	TArray<TSharedPtr<IPropertyHandle>> GetSelectedHandles() const;

	/** Returns all selected rows (editable and immutable) in list order. */
	TArray<TSharedPtr<FKzStackRow>> GetSelectedRows() const;

	/**
	 * Returns the "primary" editable handle (last clicked) or null.
	 * Convenience for consumers that don't care about multi-select.
	 */
	TSharedPtr<IPropertyHandle> GetPrimarySelectedHandle() const;

	/**
	 * Programmatically select a row by its index in the underlying
	 * array, replacing any existing selection. Returns true if applied.
	 */
	bool SelectByIndex(int32 Index);

	/**
	 * Programmatically select a row by a context GUID, asking the row customizer to
	 * resolve which handle it corresponds to. Returns true if resolved and selected.
	 */
	bool SelectByContextId(const FGuid& ContextId);

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface

private:
	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyHandleArray> ArrayHandle;

	bool bAllowDuplicates = false;
	FText ItemName;
	FText ItemNamePlural;
	FString TitlePropertyMeta;

	bool bIsObjectArray = false;
	UClass* BaseObjectClass = nullptr;

	TSharedPtr<FKzPropertyStackRowCustomizer> RowCustomizer;

	FOnSelectionChanged OnSelectionChangedDelegate;
	FOnImmutableRowSelected OnImmutableRowSelectedDelegate;
	FKzImmutableRowsProvider ImmutableRowsProvider;

	TSharedPtr<SListView<TSharedPtr<FKzStackRow>>> ListViewWidget;
	TSharedPtr<FUICommandList> CommandList;

	TArray<TSharedPtr<FKzStackRow>> AllRows;
	TArray<TSharedPtr<FKzStackRow>> FilteredRows;

	/** Array indices of the last user-driven EDITABLE selection, used to restore it across undo/redo. */
	TArray<int32> SelectedIndices;

	/** True while RestoreSelectionByIndices drives the list, so OnListSelectionChanged ignores its own churn. */
	bool bRestoringSelection = false;

	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SBox> AddWidgetContainer;

	void RefreshStack();
	void GenerateFilteredList();

	/** Re-selects the EDITABLE rows at the given array indices (skipping any now out of range). */
	void RestoreSelectionByIndices(const TArray<int32>& Indices);
	void OnSearchBoxTextChanged(const FText& InSearchText);
	FText GetSearchText() const;

	FReply OnClearAllClicked();
	bool CanClearAll() const;
	FText GetClearAllToolTip() const;

	/** Every editable element of the array, for removal checks that span it. */
	TArray<TSharedPtr<IPropertyHandle>> GetAllHandles() const;

	/** Asks the row customizer whether the rows can be removed together; OutReason, when given, says why not. */
	bool CanRemoveRows(const TArray<TSharedPtr<IPropertyHandle>>& Handles, FText* OutReason = nullptr) const;

	/** Shows why the user's removal was refused. */
	void NotifyRemovalRefused(const FText& Reason) const;

	FString GetRowDisplayName(TSharedPtr<FKzStackRow> Row) const;
	FString GetHandleDisplayName(TSharedPtr<IPropertyHandle> Handle) const;
	FText GetHandleToolTip(TSharedPtr<IPropertyHandle> Handle) const;

	/** Fires the appropriate selection delegate(s) for the current selection. */
	void NotifySelectionChanged();

	//~ Begin SWidget Interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	//~ End SWidget Interface

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FKzStackRow> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnListSelectionChanged(TSharedPtr<FKzStackRow> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnRowDoubleClick(TSharedPtr<FKzStackRow> Item);
	TSharedPtr<SWidget> GetContextMenuContent();

	FReply OnAddElementClicked();
	void OnAddObjectClassSelected(UClass* ObjectClass);
	FReply OnDeleteElementClicked(TSharedPtr<FKzStackRow> ItemToDelete);
	TSet<UClass*> GetDisallowedClasses() const;

	FReply OnRowDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedPtr<FKzStackRow> DraggedItem);
	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FKzStackRow> TargetItem);
	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FKzStackRow> TargetItem);

	void BindCommands();

	// Multi-aware command implementations.
	void CopySelectedElements();
	bool CanCopyElements() const;
	void PasteElement();
	bool CanPasteElement() const;
	void CutSelectedElements();
	bool CanCutElements() const;
	void DeleteSelectedElements();
	bool CanDeleteElements() const;
	void DuplicateSelectedElements();
	bool CanDuplicateElements() const;
};

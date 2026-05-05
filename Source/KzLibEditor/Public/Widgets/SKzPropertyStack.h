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

	SLATE_BEGIN_ARGS(SKzPropertyStack)
		: _bAllowDuplicates(false)
		{
		}
		SLATE_ARGUMENT(bool, bAllowDuplicates)
		SLATE_ARGUMENT(FText, ItemName)
		SLATE_ARGUMENT(TSharedPtr<FKzPropertyStackRowCustomizer>, RowCustomizer)
		/** Fired when the selection changes. The array contains all currently selected
		 *  handles, in the order the list view returns them (typically top-to-bottom). */
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle);
	virtual ~SKzPropertyStack();

	void SetPropertyHandle(TSharedPtr<IPropertyHandle> InPropertyHandle);

	/** Returns all selected handles in their current list order. */
	TArray<TSharedPtr<IPropertyHandle>> GetSelectedHandles() const;

	/**
	 * Returns the "primary" handle (last clicked) or null.
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
	FString TitlePropertyMeta;

	bool bIsObjectArray = false;
	UClass* BaseObjectClass = nullptr;

	TSharedPtr<FKzPropertyStackRowCustomizer> RowCustomizer;

	FOnSelectionChanged OnSelectionChangedDelegate;
	TSharedPtr<SListView<TSharedPtr<IPropertyHandle>>> ListViewWidget;
	TSharedPtr<FUICommandList> CommandList;

	TArray<TSharedPtr<IPropertyHandle>> AllHandles;
	TArray<TSharedPtr<IPropertyHandle>> FilteredHandles;

	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SBox> AddWidgetContainer;

	void RefreshStack();
	void GenerateFilteredList();
	void OnSearchBoxTextChanged(const FText& InSearchText);
	FText GetSearchText() const;

	FString GetHandleDisplayName(TSharedPtr<IPropertyHandle> Handle) const;
	FText GetHandleToolTip(TSharedPtr<IPropertyHandle> Handle) const;

	//~ Begin SWidget Interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	//~ End SWidget Interface

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<IPropertyHandle> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnListSelectionChanged(TSharedPtr<IPropertyHandle> SelectedItem, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> GetContextMenuContent();

	FReply OnAddElementClicked();
	void OnAddObjectClassSelected(UClass* ObjectClass);
	FReply OnDeleteElementClicked(TSharedPtr<IPropertyHandle> ItemToDelete);
	TSet<UClass*> GetDisallowedClasses() const;

	FReply OnRowDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedPtr<IPropertyHandle> DraggedItem);
	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem);
	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem);

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
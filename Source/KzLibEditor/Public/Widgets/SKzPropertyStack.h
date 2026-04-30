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

class FKzPropertyDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FKzPropertyDragDropOp, FDragDropOperation)

	TSharedPtr<IPropertyHandle> HandleToDrag;

	static TSharedRef<FKzPropertyDragDropOp> New(TSharedPtr<IPropertyHandle> InHandleToDrag)
	{
		TSharedRef<FKzPropertyDragDropOp> Operation = MakeShared<FKzPropertyDragDropOp>();
		Operation->HandleToDrag = InHandleToDrag;
		Operation->Construct();
		return Operation;
	}
};

class KZLIBEDITOR_API SKzPropertyStack : public SCompoundWidget, public FEditorUndoClient
{
public:
	DECLARE_DELEGATE_OneParam(FOnItemSelected, TSharedPtr<IPropertyHandle> /*SelectedHandle*/);

	SLATE_BEGIN_ARGS(SKzPropertyStack)
		: _bAllowDuplicates(false)
		{
		}
		SLATE_ARGUMENT(bool, bAllowDuplicates)
		SLATE_ARGUMENT(FText, ItemName)
		SLATE_EVENT(FOnItemSelected, OnItemSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle);
	virtual ~SKzPropertyStack();

	void SetPropertyHandle(TSharedPtr<IPropertyHandle> InPropertyHandle);
	TSharedPtr<IPropertyHandle> GetSelectedPropertyHandle() const;

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

	FOnItemSelected OnItemSelectedDelegate;
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
	void CopySelectedElement();
	bool CanCopyElement() const;
	void PasteElement();
	bool CanPasteElement() const;
	void CutSelectedElement();
	bool CanCutElement() const;
	void DeleteSelectedElement();
	bool CanDeleteElement() const;
};
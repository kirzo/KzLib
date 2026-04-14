// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzLibEditorStyle.h"
#include "Utils/KzObjectEditorUtils.h"
#include "EditorUndoClient.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "SPositiveActionButton.h"
#include "Styling/AppStyle.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateIconFinder.h"
#include "UObject/UObjectIterator.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

template<typename TItemClass>
class SKzObjectStack : public SCompoundWidget, public FEditorUndoClient
{
public:
	DECLARE_DELEGATE_OneParam(FOnItemSelected, TItemClass* /*SelectedItem*/);

	SLATE_BEGIN_ARGS(SKzObjectStack)
		: _bAllowDuplicates(false)
		{
		}
		// The UObject that owns the array (used for Modify() and as Outer for NewObject)
		SLATE_ARGUMENT(UObject*, OwnerAsset)
		// Pointer to the actual array we are modifying
		SLATE_ARGUMENT(TArray<TObjectPtr<TItemClass>>*, TargetArray)
		// Configuration: Can the array contain multiple instances of the exact same class?
		SLATE_ARGUMENT(bool, bAllowDuplicates)
		// Optional: The display name of the element type
		SLATE_ARGUMENT(FText, ItemName)
		// Callback when selection changes
		SLATE_EVENT(FOnItemSelected, OnItemSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwnerAsset = InArgs._OwnerAsset;
		TargetArray = InArgs._TargetArray;
		bAllowDuplicates = InArgs._bAllowDuplicates;
		OnItemSelectedDelegate = InArgs._OnItemSelected;

		// Default to "Element" if no custom name is provided
		ItemName = InArgs._ItemName.IsEmpty() ? INVTEXT("Element") : InArgs._ItemName;

		BindCommands();

		if (GEditor)
		{
			GEditor->RegisterForUndo(this);
		}

		RefreshList();

		ChildSlot
			[
				SNew(SVerticalBox)

					// 1. Add Header Bar
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.Padding(0.0f)
					[
						SNew(SBorder)
							.Padding(0.0f)
							.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
							[
								SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									.VAlign(VAlign_Top)
									[
										SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.VAlign(VAlign_Center)
											.HAlign(HAlign_Left)
											.FillWidth(1.f)
											.Padding(6.0f, 4.0f)
											[
												SNew(SPositiveActionButton)
													.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
													.Text(FText::Format(INVTEXT("Add {0}"), ItemName))
													.ToolTipText(FText::Format(INVTEXT("Add a new {0} to this list."), ItemName))
													.OnGetMenuContent(this, &SKzObjectStack::GetAddMenuContent)
											]
									]
							]
					]

				// 2. The List View
				+ SVerticalBox::Slot()
					.Padding(0.0f)
					.FillHeight(1.0f)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.Padding(5.0f)
							[
								SAssignNew(ListViewWidget, SListView<TItemClass*>)
									.ListItemsSource(&DisplayList)
									.OnGenerateRow(this, &SKzObjectStack::OnGenerateRowForObject)
									.OnSelectionChanged(this, &SKzObjectStack::OnObjectSelectionChanged)
									.OnContextMenuOpening(this, &SKzObjectStack::GetContextMenuContent)
							]
					]
			];
	}

	virtual ~SKzObjectStack()
	{
		if (GEditor)
		{
			GEditor->UnregisterForUndo(this);
		}
	}

	void RefreshList()
	{
		DisplayList.Empty();

		if (OwnerAsset && TargetArray)
		{
			for (TObjectPtr<TItemClass> Obj : *TargetArray)
			{
				if (Obj)
				{
					DisplayList.Add(Obj.Get());
				}
			}
		}

		if (ListViewWidget.IsValid())
		{
			ListViewWidget->RequestListRefresh();
		}
	}

	//~ Begin SWidget Interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (CommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }
	//~ End SWidget Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override
	{
		RefreshList();
		OnItemSelectedDelegate.ExecuteIfBound(nullptr);
	}

	virtual void PostRedo(bool bSuccess) override
	{
		RefreshList();
		OnItemSelectedDelegate.ExecuteIfBound(nullptr);
	}
	//~ End FEditorUndoClient Interface

private:
	UObject* OwnerAsset = nullptr;
	TArray<TObjectPtr<TItemClass>>* TargetArray = nullptr;
	bool bAllowDuplicates = false;
	FText ItemName;

	FOnItemSelected OnItemSelectedDelegate;
	TSharedPtr<SListView<TItemClass*>> ListViewWidget;
	TArray<TItemClass*> DisplayList;
	TSharedPtr<FUICommandList> CommandList;

	// --- Slate Delegates ---
	TSharedRef<ITableRow> OnGenerateRowForObject(TItemClass* Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		FText DisplayName = INVTEXT("Null Object");
		FText Tooltip = FText::GetEmpty();
		if (Item)
		{
			DisplayName = Item->GetClass()->GetDisplayNameText();
			Tooltip = Item->GetClass()->GetToolTipText();
		}

		const float HorizontalPadding = 6.0f;
		const float VerticalPadding = 3.0f;
		const FMargin Margin(HorizontalPadding, VerticalPadding);

		return SNew(STableRow<TItemClass*>, OwnerTable)
			.ShowSelection(false)
			.Padding(Margin)
			.OnDragDetected(this, &SKzObjectStack::OnRowDragDetected, Item)
			.OnCanAcceptDrop(this, &SKzObjectStack::OnCanAcceptDrop)
			.OnAcceptDrop(this, &SKzObjectStack::OnAcceptDrop)
			[
				SNew(SBorder)
					.ToolTipText(Tooltip)
					.BorderImage_Lambda([this, Item]()
						{
							if (ListViewWidget.IsValid() && ListViewWidget->IsItemSelected(Item))
							{
								return FKzLibEditorStyle::Get().GetBrush("Kz.CardBorderSelected");
							}
							return FKzLibEditorStyle::Get().GetBrush("Kz.CardBorder");
						})
					.Padding(Margin * 1.5f)
					[
						SNew(SHorizontalBox)

							// Drag Icon
							+ SHorizontalBox::Slot()
							.MaxWidth(18)
							.AutoWidth()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(Margin)
							[
								SNew(SImage)
									.Image(FAppStyle::GetBrush("VerticalBoxDragIndicatorShort"))
							]

							// Display Name
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(Margin)
							[
								SNew(STextBlock)
									.Text(DisplayName)
							]

							// Spacer
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Fill)
							.FillWidth(1.0f)
							[
								SNew(SSpacer)
									.Size(FVector2D(0.0f, 1.0f))
							]

							// Delete Button
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Center)
							.Padding(Margin)
							[
								SNew(SButton)
									.ContentPadding(FMargin(0, 2))
									.ToolTipText(FText::Format(INVTEXT("Delete this {0}."), ItemName))
									.OnClicked(this, &SKzObjectStack::OnDeleteObjectClicked, Item)
									.Content()
									[
										SNew(SImage)
											.Image(FAppStyle::GetBrush("Icons.Delete"))
											.ColorAndOpacity(FSlateColor::UseForeground())
									]
							]
					]
			];
	}

	void OnObjectSelectionChanged(TItemClass* Item, ESelectInfo::Type SelectInfo)
	{
		OnItemSelectedDelegate.ExecuteIfBound(Item);
	}

	TSharedPtr<SWidget> GetContextMenuContent()
	{
		if (!CanCopyObject() && !CanPasteObject())
		{
			return nullptr;
		}

		FMenuBuilder MenuBuilder(true, CommandList);

		MenuBuilder.BeginSection("ElementActions", INVTEXT("Actions"));
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	// --- Drag & Drop ---
	FReply OnRowDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TItemClass* DraggedObject)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			return FReply::Handled().BeginDragDrop(TKzObjectDragDropOp<TItemClass>::New(DraggedObject));
		}
		return FReply::Unhandled();
	}

	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TItemClass* TargetObject)
	{
		TSharedPtr<TKzObjectDragDropOp<TItemClass>> DragOp = DragDropEvent.GetOperationAs<TKzObjectDragDropOp<TItemClass>>();

		if (DragOp.IsValid() && DragOp->ObjectToDrag != TargetObject)
		{
			return DropZone;
		}
		return TOptional<EItemDropZone>();
	}

	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TItemClass* TargetObject)
	{
		TSharedPtr<TKzObjectDragDropOp<TItemClass>> DragOp = DragDropEvent.GetOperationAs<TKzObjectDragDropOp<TItemClass>>();

		if (DragOp.IsValid() && OwnerAsset && TargetArray && DragOp->ObjectToDrag != TargetObject)
		{
			int32 OldIndex = TargetArray->Find(DragOp->ObjectToDrag);
			int32 NewIndex = TargetArray->Find(TargetObject);

			if (OldIndex != INDEX_NONE && NewIndex != INDEX_NONE)
			{
				const FScopedTransaction Transaction(FText::Format(INVTEXT("Reorder {0}"), ItemName));
				OwnerAsset->Modify();

				if (DropZone == EItemDropZone::BelowItem)
				{
					NewIndex++;
				}

				TItemClass* ObjectToMove = DragOp->ObjectToDrag;
				TargetArray->RemoveAt(OldIndex);

				if (OldIndex < NewIndex)
				{
					NewIndex--;
				}

				TargetArray->Insert(ObjectToMove, NewIndex);
				RefreshList();

				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}

	// --- Actions ---
	TSharedRef<SWidget> GetAddMenuContent()
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		if (!OwnerAsset || !TargetArray)
		{
			return MenuBuilder.MakeWidget();
		}

		TSet<UClass*> ExistingClasses;
		if (!bAllowDuplicates)
		{
			for (TObjectPtr<TItemClass> Obj : *TargetArray)
			{
				if (Obj)
				{
					ExistingClasses.Add(Obj->GetClass());
				}
			}
		}

		bool bHasValidClasses = false;

		MenuBuilder.BeginSection("AvailableElements", FText::Format(INVTEXT("{0}s"), ItemName));
		{
			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* Class = *ClassIt;

				if (Class->IsChildOf(TItemClass::StaticClass()) &&
					!Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) &&
					!Class->GetName().StartsWith(TEXT("SKEL_")) &&
					!Class->GetName().StartsWith(TEXT("REINST_")))
				{
					if (bAllowDuplicates || !ExistingClasses.Contains(Class))
					{
						bHasValidClasses = true;
						MenuBuilder.AddMenuEntry(
							Class->GetDisplayNameText(),
							Class->GetToolTipText(),
							FSlateIconFinder::FindIconForClass(Class),
							FUIAction(FExecuteAction::CreateSP(this, &SKzObjectStack::OnAddObjectClassSelected, Class))
						);
					}
				}
			}
		}
		MenuBuilder.EndSection();

		if (!bHasValidClasses)
		{
			MenuBuilder.AddWidget(
				SNew(STextBlock)
				.Text(FText::Format(INVTEXT("No more {0}s available"), ItemName))
				.Margin(FMargin(4.0f)),
				FText::GetEmpty()
			);
		}

		return MenuBuilder.MakeWidget();
	}

	void OnAddObjectClassSelected(UClass* ObjectClass)
	{
		if (OwnerAsset && TargetArray && ObjectClass)
		{
			const FScopedTransaction Transaction(FText::Format(INVTEXT("Add {0}"), ItemName));
			OwnerAsset->Modify();

			TItemClass* NewObj = NewObject<TItemClass>(OwnerAsset, ObjectClass, NAME_None, RF_Transactional);
			TargetArray->Add(NewObj);

			RefreshList();

			if (ListViewWidget.IsValid())
			{
				ListViewWidget->SetSelection(NewObj);
			}
			OnItemSelectedDelegate.ExecuteIfBound(NewObj);
		}
	}

	FReply OnDeleteObjectClicked(TItemClass* ObjectToDelete)
	{
		if (OwnerAsset && TargetArray && ObjectToDelete)
		{
			const FScopedTransaction Transaction(FText::Format(INVTEXT("Delete {0}"), ItemName));

			OwnerAsset->Modify();
			TargetArray->Remove(ObjectToDelete);

			OnItemSelectedDelegate.ExecuteIfBound(nullptr);
			RefreshList();
		}
		return FReply::Handled();
	}

	// --- Clipboard Commands ---
	void BindCommands()
	{
		CommandList = MakeShared<FUICommandList>();

		CommandList->MapAction(
			FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &SKzObjectStack::CopySelectedObject),
			FCanExecuteAction::CreateSP(this, &SKzObjectStack::CanCopyObject)
		);

		CommandList->MapAction(
			FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &SKzObjectStack::PasteObject),
			FCanExecuteAction::CreateSP(this, &SKzObjectStack::CanPasteObject)
		);

		CommandList->MapAction(
			FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &SKzObjectStack::CutSelectedObject),
			FCanExecuteAction::CreateSP(this, &SKzObjectStack::CanCutObject)
		);

		CommandList->MapAction(
			FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &SKzObjectStack::DeleteSelectedObject),
			FCanExecuteAction::CreateSP(this, &SKzObjectStack::CanDeleteObject)
		);
	}

	void CopySelectedObject()
	{
		TArray<TItemClass*> SelectedItems = ListViewWidget->GetSelectedItems();
		if (SelectedItems.Num() > 0 && SelectedItems[0])
		{
			FKzClipboardUtils::CopyObjectToClipboard(SelectedItems[0]);
		}
	}

	bool CanCopyObject() const
	{
		return ListViewWidget.IsValid() && ListViewWidget->GetNumItemsSelected() > 0;
	}

	void PasteObject()
	{
		TItemClass* PastedObject = FKzClipboardUtils::PasteObjectFromClipboard<TItemClass>(OwnerAsset);

		if (PastedObject && OwnerAsset && TargetArray)
		{
			bool bExists = false;
			if (!bAllowDuplicates)
			{
				for (TItemClass* Obj : *TargetArray)
				{
					if (Obj && Obj->GetClass() == PastedObject->GetClass())
					{
						bExists = true;
						break;
					}
				}
			}

			if (bExists)
			{
				FNotificationInfo Info(FText::Format(INVTEXT("{0} of type '{1}' already exists."), ItemName, PastedObject->GetClass()->GetDisplayNameText()));
				Info.ExpireDuration = 3.0f;
				Info.Image = FAppStyle::GetBrush("Icons.Warning");
				FSlateNotificationManager::Get().AddNotification(Info);

				PastedObject->ClearFlags(RF_Transactional);
				PastedObject->MarkAsGarbage();
			}
			else
			{
				const FScopedTransaction Transaction(FText::Format(INVTEXT("Paste {0}"), ItemName));
				OwnerAsset->Modify();

				TargetArray->Add(PastedObject);
				RefreshList();

				if (ListViewWidget.IsValid())
				{
					ListViewWidget->SetSelection(PastedObject);
				}
				OnItemSelectedDelegate.ExecuteIfBound(PastedObject);
			}
		}
	}

	bool CanPasteObject() const
	{
		return true;
	}

	void CutSelectedObject()
	{
		CopySelectedObject();
		DeleteSelectedObject();
	}

	bool CanCutObject() const
	{
		return CanCopyObject();
	}

	void DeleteSelectedObject()
	{
		TArray<TItemClass*> SelectedItems = ListViewWidget->GetSelectedItems();
		if (SelectedItems.Num() > 0 && SelectedItems[0])
		{
			OnDeleteObjectClicked(SelectedItems[0]);
		}
	}

	bool CanDeleteObject() const
	{
		return CanCopyObject();
	}
};
// Copyright 2026 kirzo

#include "Widgets/SKzPropertyStack.h"
#include "PropertyHandle.h"
#include "Widgets/SKzClassCombo.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "KzLibEditorStyle.h"
#include "Utils/KzObjectEditorUtils.h"

void SKzPropertyStack::Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	bAllowDuplicates = InArgs._bAllowDuplicates;
	OnItemSelectedDelegate = InArgs._OnItemSelected;
	ItemName = InArgs._ItemName.IsEmpty() ? INVTEXT("Element") : InArgs._ItemName;

	TextFilter = MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString);

	SAssignNew(SearchBox, SSearchBox)
		.HintText(FText::Format(INVTEXT("Search {0}s"), ItemName))
		.OnTextChanged(this, &SKzPropertyStack::OnSearchBoxTextChanged);

	BindCommands();

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	ChildSlot
		[
			SNew(SVerticalBox)

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
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Left)
								.AutoWidth()
								.Padding(6.0f, 4.0f)
								[
									SAssignNew(AddWidgetContainer, SBox)
								]

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								.Padding(3.0f, 3.0f)
								[
									SearchBox.ToSharedRef()
								]
						]
				]

			+ SVerticalBox::Slot()
				.Padding(0.0f)
				.FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(5.0f)
						[
							SAssignNew(ListViewWidget, SListView<TSharedPtr<IPropertyHandle>>)
								.ListItemsSource(&FilteredHandles)
								.OnGenerateRow(this, &SKzPropertyStack::OnGenerateRow)
								.OnSelectionChanged(this, &SKzPropertyStack::OnListSelectionChanged)
								.OnContextMenuOpening(this, &SKzPropertyStack::GetContextMenuContent)
								.SelectionMode(ESelectionMode::Single)
						]
				]
		];

	SetPropertyHandle(InPropertyHandle);
}

SKzPropertyStack::~SKzPropertyStack()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void SKzPropertyStack::PostUndo(bool bSuccess)
{
	RefreshStack();
	OnItemSelectedDelegate.ExecuteIfBound(nullptr);
}

void SKzPropertyStack::PostRedo(bool bSuccess)
{
	RefreshStack();
	OnItemSelectedDelegate.ExecuteIfBound(nullptr);
}

void SKzPropertyStack::RefreshStack()
{
	AllHandles.Empty();

	if (ArrayHandle.IsValid())
	{
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);

		for (uint32 i = 0; i < NumElements; ++i)
		{
			AllHandles.Add(ArrayHandle->GetElement(i));
		}
	}

	GenerateFilteredList();
}

void SKzPropertyStack::GenerateFilteredList()
{
	FilteredHandles.Empty();
	const bool bHasFilterText = !TextFilter->GetFilterText().IsEmpty();

	for (TSharedPtr<IPropertyHandle> Handle : AllHandles)
	{
		if (Handle.IsValid())
		{
			bool bPassesFilter = true;
			if (bHasFilterText)
			{
				const FString ItemNameStr = GetHandleDisplayName(Handle);
				bPassesFilter = TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(ItemNameStr));
			}

			if (bPassesFilter)
			{
				FilteredHandles.Add(Handle);
			}
		}
	}

	if (ListViewWidget.IsValid())
	{
		ListViewWidget->RequestListRefresh();
	}
}

FString SKzPropertyStack::GetHandleDisplayName(TSharedPtr<IPropertyHandle> Handle) const
{
	if (!Handle.IsValid()) return TEXT("Invalid");

	// 1. Try to use the TitleProperty meta if defined
	if (!TitlePropertyMeta.IsEmpty())
	{
		TSharedPtr<IPropertyHandle> TitleHandle = Handle->GetChildHandle(*TitlePropertyMeta);
		if (TitleHandle.IsValid())
		{
			FString TitleValue;
			// GetValueAsDisplayString formats FText, FName, numeric values, etc., cleanly for UI
			if (TitleHandle->GetValueAsDisplayString(TitleValue) == FPropertyAccess::Success && !TitleValue.IsEmpty())
			{
				return TitleValue;
			}
		}
	}

	// 2. Try to use the Object Class Name
	if (bIsObjectArray)
	{
		UObject* ObjectValue = nullptr;
		if (Handle->GetValue(ObjectValue) == FPropertyAccess::Success && ObjectValue)
		{
			return ObjectValue->GetClass()->GetDisplayNameText().ToString();
		}
	}

	// 3. Fallback to generic index name
	return FString::Printf(TEXT("%s %s"), *ItemName.ToString(), *Handle->GetPropertyDisplayName().ToString());
}

FText SKzPropertyStack::GetHandleToolTip(TSharedPtr<IPropertyHandle> Handle) const
{
	if (!Handle.IsValid()) return FText::GetEmpty();

	if (bIsObjectArray)
	{
		UObject* ObjectValue = nullptr;
		if (Handle->GetValue(ObjectValue) == FPropertyAccess::Success && ObjectValue)
		{
			return ObjectValue->GetClass()->GetToolTipText();
		}
	}

	return Handle->GetToolTipText();
}

void SKzPropertyStack::OnSearchBoxTextChanged(const FText& InSearchText)
{
	TextFilter->SetFilterText(InSearchText);
	SearchBox->SetError(TextFilter->GetFilterErrorText());
	GenerateFilteredList();
}

FText SKzPropertyStack::GetSearchText() const
{
	return TextFilter->GetFilterText();
}

FReply SKzPropertyStack::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SKzPropertyStack::SetPropertyHandle(TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	RootHandle = InPropertyHandle;

	if (RootHandle.IsValid())
	{
		ArrayHandle = RootHandle->AsArray();
	}
	else
	{
		ArrayHandle.Reset();
	}

	bIsObjectArray = false;
	BaseObjectClass = nullptr;
	TitlePropertyMeta.Empty();

	if (RootHandle.IsValid() && RootHandle->GetProperty())
	{
		// Extract TitleProperty meta if it exists
		if (RootHandle->GetProperty()->HasMetaData(TEXT("TitleProperty")))
		{
			TitlePropertyMeta = RootHandle->GetProperty()->GetMetaData(TEXT("TitleProperty"));
		}

		if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(RootHandle->GetProperty()))
		{
			if (FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(ArrayProp->Inner))
			{
				bIsObjectArray = true;
				BaseObjectClass = ObjectProp->PropertyClass;
			}
		}
	}

	if (AddWidgetContainer.IsValid())
	{
		if (bIsObjectArray && BaseObjectClass)
		{
			AddWidgetContainer->SetContent(
				SNew(SKzClassCombo)
				.BaseClass(BaseObjectClass)
				.ItemName(ItemName)
				.OnGetDisallowedClasses(this, &SKzPropertyStack::GetDisallowedClasses)
				.OnClassSelected(this, &SKzPropertyStack::OnAddObjectClassSelected)
			);
		}
		else if (RootHandle.IsValid())
		{
			AddWidgetContainer->SetContent(
				SNew(SButton)
				.Text(FText::Format(INVTEXT("Add {0}"), ItemName))
				.OnClicked(this, &SKzPropertyStack::OnAddElementClicked)
			);
		}
	}

	if (ArrayHandle.IsValid())
	{
		ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &SKzPropertyStack::RefreshStack));
	}

	RefreshStack();
}

TSharedPtr<IPropertyHandle> SKzPropertyStack::GetSelectedPropertyHandle() const
{
	if (ListViewWidget.IsValid())
	{
		TArray<TSharedPtr<IPropertyHandle>> SelectedItems = ListViewWidget->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			return SelectedItems[0];
		}
	}
	return nullptr;
}

TSharedRef<ITableRow> SKzPropertyStack::OnGenerateRow(TSharedPtr<IPropertyHandle> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText Tooltip = GetHandleToolTip(Item);

	const float HorizontalPadding = 6.0f;
	const float VerticalPadding = 3.0f;
	const FMargin Margin(HorizontalPadding, VerticalPadding);

	return SNew(STableRow<TSharedPtr<IPropertyHandle>>, OwnerTable)
		.ShowSelection(false)
		.Padding(Margin)
		.OnDragDetected(this, &SKzPropertyStack::OnRowDragDetected, Item)
		.OnCanAcceptDrop(this, &SKzPropertyStack::OnCanAcceptDrop)
		.OnAcceptDrop(this, &SKzPropertyStack::OnAcceptDrop)
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

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.Padding(Margin)
						[
							SNew(STextBlock)
								.Text_Lambda([this, Item]() { return FText::FromString(GetHandleDisplayName(Item)); })
								.HighlightText(this, &SKzPropertyStack::GetSearchText)
						]

						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Fill)
						.FillWidth(1.0f)
						[
							SNew(SSpacer)
								.Size(FVector2D(0.0f, 1.0f))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.Padding(Margin)
						[
							SNew(SButton)
								.ContentPadding(FMargin(0, 2))
								.ToolTipText(FText::Format(INVTEXT("Delete this {0}."), ItemName))
								.OnClicked(this, &SKzPropertyStack::OnDeleteElementClicked, Item)
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

void SKzPropertyStack::OnListSelectionChanged(TSharedPtr<IPropertyHandle> SelectedItem, ESelectInfo::Type SelectInfo)
{
	OnItemSelectedDelegate.ExecuteIfBound(SelectedItem);
}

TSharedPtr<SWidget> SKzPropertyStack::GetContextMenuContent()
{
	if (!CanCopyElement() && !CanPasteElement())
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

TSet<UClass*> SKzPropertyStack::GetDisallowedClasses() const
{
	TSet<UClass*> Disallowed;
	if (!bAllowDuplicates && bIsObjectArray && ArrayHandle.IsValid())
	{
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		for (uint32 i = 0; i < NumElements; ++i)
		{
			TSharedPtr<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
			if (ElementHandle.IsValid())
			{
				UObject* Obj = nullptr;
				if (ElementHandle->GetValue(Obj) == FPropertyAccess::Success && Obj)
				{
					Disallowed.Add(Obj->GetClass());
				}
			}
		}
	}
	return Disallowed;
}

FReply SKzPropertyStack::OnAddElementClicked()
{
	if (ArrayHandle.IsValid())
	{
		const FScopedTransaction Transaction(FText::Format(INVTEXT("Add {0}"), ItemName));
		ArrayHandle->AddItem();
		RefreshStack();

		if (AllHandles.Num() > 0 && ListViewWidget.IsValid())
		{
			ListViewWidget->SetSelection(AllHandles.Last());
			OnItemSelectedDelegate.ExecuteIfBound(AllHandles.Last());
		}
	}
	return FReply::Handled();
}

void SKzPropertyStack::OnAddObjectClassSelected(UClass* ObjectClass)
{
	if (!ArrayHandle.IsValid() || !ObjectClass || !RootHandle.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Add {0}"), ItemName));

	TArray<UObject*> OuterObjects;
	RootHandle->GetOuterObjects(OuterObjects);
	UObject* OuterAsset = OuterObjects.Num() > 0 ? OuterObjects[0] : GetTransientPackage();

	if (OuterAsset)
	{
		OuterAsset->Modify();
	}

	ArrayHandle->AddItem();

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	if (NumElements == 0) return;

	TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);

	if (NewElementHandle.IsValid())
	{
		UObject* NewObj = NewObject<UObject>(OuterAsset, ObjectClass, NAME_None, RF_Transactional);

		NewElementHandle->SetValue(NewObj);

		UObject* CheckObj = nullptr;
		NewElementHandle->GetValue(CheckObj);

		if (CheckObj != NewObj)
		{
			if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(NewElementHandle->GetProperty()))
			{
				TArray<void*> RawData;
				NewElementHandle->AccessRawData(RawData);
				for (void* DataPtr : RawData)
				{
					ObjProp->SetObjectPropertyValue(DataPtr, NewObj);
				}
			}
		}

		RefreshStack();

		if (ListViewWidget.IsValid())
		{
			ListViewWidget->SetSelection(NewElementHandle);
		}
		OnItemSelectedDelegate.ExecuteIfBound(NewElementHandle);
	}
}

FReply SKzPropertyStack::OnDeleteElementClicked(TSharedPtr<IPropertyHandle> ItemToDelete)
{
	if (ArrayHandle.IsValid() && ItemToDelete.IsValid())
	{
		const FScopedTransaction Transaction(FText::Format(INVTEXT("Delete {0}"), ItemName));
		ArrayHandle->DeleteItem(ItemToDelete->GetIndexInArray());
		RefreshStack();
		OnItemSelectedDelegate.ExecuteIfBound(nullptr);
	}
	return FReply::Handled();
}

FReply SKzPropertyStack::OnRowDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedPtr<IPropertyHandle> DraggedItem)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().BeginDragDrop(FKzPropertyDragDropOp::New(DraggedItem));
	}
	return FReply::Unhandled();
}

TOptional<EItemDropZone> SKzPropertyStack::OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();

	if (DragOp.IsValid() && DragOp->HandleToDrag != TargetItem)
	{
		return DropZone;
	}
	return TOptional<EItemDropZone>();
}

FReply SKzPropertyStack::OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();

	if (DragOp.IsValid() && ArrayHandle.IsValid() && DragOp->HandleToDrag != TargetItem)
	{
		int32 OldIndex = DragOp->HandleToDrag->GetIndexInArray();
		int32 NewIndex = TargetItem->GetIndexInArray();

		if (OldIndex != INDEX_NONE && NewIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction(FText::Format(INVTEXT("Reorder {0}"), ItemName));

			// Mark the outer assets as modified so the editor knows it needs saving
			if (RootHandle.IsValid())
			{
				TArray<UObject*> OuterObjects;
				RootHandle->GetOuterObjects(OuterObjects);
				for (UObject* OuterObj : OuterObjects)
				{
					if (OuterObj)
					{
						OuterObj->Modify();
					}
				}
			}

			if (DropZone == EItemDropZone::BelowItem)
			{
				NewIndex++;
			}
			if (OldIndex < NewIndex)
			{
				NewIndex--;
			}

			ArrayHandle->MoveElementTo(OldIndex, NewIndex);
			RefreshStack();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

void SKzPropertyStack::BindCommands()
{
	CommandList = MakeShared<FUICommandList>();

	CommandList->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::CopySelectedElement),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanCopyElement));

	CommandList->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::PasteElement),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanPasteElement));

	CommandList->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::CutSelectedElement),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanCutElement));

	CommandList->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::DeleteSelectedElement),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanDeleteElement));
}

void SKzPropertyStack::CopySelectedElement()
{
	TSharedPtr<IPropertyHandle> Selected = GetSelectedPropertyHandle();
	if (Selected.IsValid())
	{
		if (bIsObjectArray)
		{
			// Deep copy for UObjects using your custom exporter
			UObject* ObjectToCopy = nullptr;
			if (Selected->GetValue(ObjectToCopy) == FPropertyAccess::Success && ObjectToCopy)
			{
				FKzClipboardUtils::CopyObjectToClipboard(ObjectToCopy);
			}
		}
		else
		{
			// Generic string copy for structs and primitive types
			FString SerializedValue;
			if (Selected->GetValueAsFormattedString(SerializedValue) == FPropertyAccess::Success)
			{
				FPlatformApplicationMisc::ClipboardCopy(*SerializedValue);
			}
		}
	}
}

bool SKzPropertyStack::CanCopyElement() const
{
	return GetSelectedPropertyHandle().IsValid();
}

void SKzPropertyStack::PasteElement()
{
	if (!ArrayHandle.IsValid() || !RootHandle.IsValid())
	{
		return;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	if (ClipboardContent.IsEmpty())
	{
		return;
	}

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Paste {0}"), ItemName));

	if (bIsObjectArray && BaseObjectClass)
	{
		// 1. Setup Outer for the new object
		TArray<UObject*> OuterObjects;
		RootHandle->GetOuterObjects(OuterObjects);
		UObject* OuterAsset = OuterObjects.Num() > 0 ? OuterObjects[0] : GetTransientPackage();

		if (OuterAsset)
		{
			OuterAsset->Modify();
		}

		// 2. Perform deep instantiation from clipboard
		UObject* PastedObject = FKzClipboardUtils::PasteObjectFromClipboard(OuterAsset);

		if (PastedObject)
		{
			// Validate that the pasted object inherits from our allowed BaseClass
			if (!PastedObject->IsA(BaseObjectClass))
			{
				PastedObject->ClearFlags(RF_Transactional);
				PastedObject->MarkAsGarbage();
				return;
			}

			// 3. Duplicate check logic
			bool bExists = false;
			if (!bAllowDuplicates)
			{
				uint32 NumElements = 0;
				ArrayHandle->GetNumElements(NumElements);
				for (uint32 i = 0; i < NumElements; ++i)
				{
					TSharedPtr<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
					if (ElementHandle.IsValid())
					{
						UObject* ExistingObj = nullptr;
						if (ElementHandle->GetValue(ExistingObj) == FPropertyAccess::Success && ExistingObj)
						{
							if (ExistingObj->GetClass() == PastedObject->GetClass())
							{
								bExists = true;
								break;
							}
						}
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
				// 4. Inject the deep-copied object into the array
				ArrayHandle->AddItem();

				uint32 NumElements = 0;
				ArrayHandle->GetNumElements(NumElements);
				TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);

				if (NewElementHandle.IsValid())
				{
					NewElementHandle->SetValue(PastedObject);

					// Force raw data injection just in case Unreal's standard SetValue misbehaves with Instanced objects
					UObject* CheckObj = nullptr;
					NewElementHandle->GetValue(CheckObj);

					if (CheckObj != PastedObject)
					{
						if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(NewElementHandle->GetProperty()))
						{
							TArray<void*> RawData;
							NewElementHandle->AccessRawData(RawData);
							for (void* DataPtr : RawData)
							{
								ObjProp->SetObjectPropertyValue(DataPtr, PastedObject);
							}
						}
					}

					RefreshStack();

					if (ListViewWidget.IsValid())
					{
						ListViewWidget->SetSelection(NewElementHandle);
					}
					OnItemSelectedDelegate.ExecuteIfBound(NewElementHandle);
				}
			}
		}
	}
	else
	{
		// Generic Paste behavior for non-object arrays (Structs/Primitives)
		ArrayHandle->AddItem();

		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);

		TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);
		if (NewElementHandle.IsValid())
		{
			NewElementHandle->SetValueFromFormattedString(ClipboardContent);
		}

		RefreshStack();

		if (ListViewWidget.IsValid())
		{
			ListViewWidget->SetSelection(NewElementHandle);
			OnItemSelectedDelegate.ExecuteIfBound(NewElementHandle);
		}
	}
}

bool SKzPropertyStack::CanPasteElement() const
{
	return ArrayHandle.IsValid();
}

void SKzPropertyStack::CutSelectedElement()
{
	const FScopedTransaction Transaction(FText::Format(INVTEXT("Cut {0}"), ItemName));
	CopySelectedElement();
	DeleteSelectedElement();
}

bool SKzPropertyStack::CanCutElement() const
{
	return CanCopyElement();
}

void SKzPropertyStack::DeleteSelectedElement()
{
	TSharedPtr<IPropertyHandle> Selected = GetSelectedPropertyHandle();
	if (Selected.IsValid())
	{
		OnDeleteElementClicked(Selected);
	}
}

bool SKzPropertyStack::CanDeleteElement() const
{
	return CanCopyElement();
}
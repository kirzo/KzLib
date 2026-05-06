// Copyright 2026 kirzo

#include "Widgets/SKzPropertyStack.h"

#include "Widgets/KzPropertyStackRowCustomizer.h"
#include "Widgets/SKzClassCombo.h"
#include "Utils/KzObjectEditorUtils.h"
#include "KzLibEditorStyle.h"

#include "PropertyHandle.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STableRow.h"
#include "SPositiveActionButton.h"

#define LOCTEXT_NAMESPACE "SKzPropertyStack"

void SKzPropertyStack::Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	bAllowDuplicates = InArgs._bAllowDuplicates;
	OnSelectionChangedDelegate = InArgs._OnSelectionChanged;
	ItemName = InArgs._ItemName.IsEmpty() ? INVTEXT("Element") : InArgs._ItemName;
	ItemNamePlural = InArgs._ItemNamePlural.IsEmpty()
		? FText::Format(INVTEXT("{0}s"), ItemName)
		: InArgs._ItemNamePlural;
	RowCustomizer = InArgs._RowCustomizer;

	TextFilter = MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString);

	SAssignNew(SearchBox, SSearchBox)
		.HintText(FText::Format(INVTEXT("Search {0}"), ItemNamePlural))
		.OnTextChanged(this, &SKzPropertyStack::OnSearchBoxTextChanged);

	BindCommands();

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	// Multi-selection unless the customizer vetoes it.
	const ESelectionMode::Type SelectionMode =
		(RowCustomizer.IsValid() && !RowCustomizer->AllowsMultiSelect())
		? ESelectionMode::Single
		: ESelectionMode::Multi;

	ChildSlot
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Top).Padding(0.0f)
				[
					SNew(SBorder)
						.Padding(0.0f)
						.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
						.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center).HAlign(HAlign_Left).AutoWidth().Padding(6.0f, 4.0f)
								[
									SAssignNew(AddWidgetContainer, SBox)
								]

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f).VAlign(VAlign_Center).Padding(3.0f, 3.0f)
								[
									SearchBox.ToSharedRef()
								]
						]
				]

			+ SVerticalBox::Slot().Padding(0.0f).FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(5.0f)
						[
							SAssignNew(ListViewWidget, SListView<TSharedPtr<IPropertyHandle>>)
								.ListItemsSource(&FilteredHandles)
								.OnGenerateRow(this, &SKzPropertyStack::OnGenerateRow)
								.OnSelectionChanged(this, &SKzPropertyStack::OnListSelectionChanged)
								.OnContextMenuOpening(this, &SKzPropertyStack::GetContextMenuContent)
								.SelectionMode(SelectionMode)
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

void SKzPropertyStack::PostUndo(bool /*bSuccess*/)
{
	RefreshStack();
	OnSelectionChangedDelegate.ExecuteIfBound({});
}

void SKzPropertyStack::PostRedo(bool /*bSuccess*/)
{
	RefreshStack();
	OnSelectionChangedDelegate.ExecuteIfBound({});
}

// =======================================================================================
// Refresh / filter
// =======================================================================================

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
		if (!Handle.IsValid()) { continue; }

		bool bPassesFilter = true;
		if (bHasFilterText)
		{
			const FString ItemNameStr = GetHandleDisplayName(Handle);
			bPassesFilter = TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(ItemNameStr));
		}

		if (bPassesFilter) { FilteredHandles.Add(Handle); }
	}

	if (ListViewWidget.IsValid()) { ListViewWidget->RequestListRefresh(); }
}

FString SKzPropertyStack::GetHandleDisplayName(TSharedPtr<IPropertyHandle> Handle) const
{
	if (!Handle.IsValid()) return TEXT("Invalid");

	if (RowCustomizer.IsValid())
	{
		const FText Override = RowCustomizer->GetDisplayText(Handle);
		if (!Override.IsEmpty()) { return Override.ToString(); }
	}

	if (!TitlePropertyMeta.IsEmpty())
	{
		TSharedPtr<IPropertyHandle> TitleHandle = Handle->GetChildHandle(*TitlePropertyMeta);
		if (TitleHandle.IsValid())
		{
			FString TitleValue;
			if (TitleHandle->GetValueAsDisplayString(TitleValue) == FPropertyAccess::Success && !TitleValue.IsEmpty())
			{
				return TitleValue;
			}
		}
	}

	if (bIsObjectArray)
	{
		UObject* ObjectValue = nullptr;
		if (Handle->GetValue(ObjectValue) == FPropertyAccess::Success && ObjectValue)
		{
			return ObjectValue->GetClass()->GetDisplayNameText().ToString();
		}
	}

	return FString::Printf(TEXT("%s %s"), *ItemName.ToString(), *Handle->GetPropertyDisplayName().ToString());
}

FText SKzPropertyStack::GetHandleToolTip(TSharedPtr<IPropertyHandle> Handle) const
{
	if (!Handle.IsValid()) return FText::GetEmpty();

	if (RowCustomizer.IsValid())
	{
		const FText Override = RowCustomizer->GetTooltipText(Handle);
		if (!Override.IsEmpty()) { return Override; }
	}

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
	ArrayHandle = RootHandle.IsValid() ? RootHandle->AsArray() : nullptr;

	bIsObjectArray = false;
	BaseObjectClass = nullptr;
	TitlePropertyMeta.Empty();

	if (RootHandle.IsValid() && RootHandle->GetProperty())
	{
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
		if (RowCustomizer.IsValid() && RowCustomizer->HasAddMenu())
		{
			AddWidgetContainer->SetContent(
				SNew(SPositiveActionButton)
				.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
				.Text(FText::Format(INVTEXT("Add {0}"), ItemName))
				//.OnComboBoxOpened(this, &SKzClassCombo::OnMenuOpened)
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
					{
						if (RowCustomizer.IsValid())
						{
							if (TSharedPtr<SWidget> Menu = RowCustomizer->BuildAddMenu(ArrayHandle))
							{
								return Menu.ToSharedRef();
							}
						}
						return SNullWidget::NullWidget;
					})
			);
		}
		else if (bIsObjectArray && BaseObjectClass)
		{
			AddWidgetContainer->SetContent(
				SNew(SKzClassCombo)
				.BaseClass(BaseObjectClass)
				.ItemName(ItemName)
				.OnGetDisallowedClasses(this, &SKzPropertyStack::GetDisallowedClasses)
				.OnClassSelected(this, &SKzPropertyStack::OnAddObjectClassSelected));
		}
		else if (RootHandle.IsValid())
		{
			AddWidgetContainer->SetContent(
				SNew(SPositiveActionButton)
				.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
				.Text(FText::Format(INVTEXT("Add {0}"), ItemName))
				.OnClicked(this, &SKzPropertyStack::OnAddElementClicked));
		}
	}

	if (ArrayHandle.IsValid())
	{
		ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &SKzPropertyStack::RefreshStack));
	}

	RefreshStack();
}

// =======================================================================================
// Selection
// =======================================================================================

TArray<TSharedPtr<IPropertyHandle>> SKzPropertyStack::GetSelectedHandles() const
{
	if (!ListViewWidget.IsValid()) { return {}; }
	return ListViewWidget->GetSelectedItems();
}

TSharedPtr<IPropertyHandle> SKzPropertyStack::GetPrimarySelectedHandle() const
{
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	return Selected.Num() > 0 ? Selected.Last() : nullptr;
}

bool SKzPropertyStack::SelectByIndex(int32 Index)
{
	if (!ListViewWidget.IsValid() || !AllHandles.IsValidIndex(Index)) { return false; }

	TSharedPtr<IPropertyHandle> Target = AllHandles[Index];
	ListViewWidget->ClearSelection();
	ListViewWidget->SetSelection(Target);
	ListViewWidget->RequestScrollIntoView(Target);
	OnSelectionChangedDelegate.ExecuteIfBound({ Target });
	return true;
}

bool SKzPropertyStack::SelectByContextId(const FGuid& ContextId)
{
	if (!RowCustomizer.IsValid() || !ListViewWidget.IsValid()) { return false; }

	TSharedPtr<IPropertyHandle> Resolved;
	if (!RowCustomizer->TryResolveContextId(ContextId, AllHandles, Resolved)) { return false; }
	if (!Resolved.IsValid()) { return false; }

	ListViewWidget->ClearSelection();
	ListViewWidget->SetSelection(Resolved);
	ListViewWidget->RequestScrollIntoView(Resolved);
	OnSelectionChangedDelegate.ExecuteIfBound({ Resolved });
	return true;
}

void SKzPropertyStack::OnListSelectionChanged(TSharedPtr<IPropertyHandle> /*SelectedItem*/, ESelectInfo::Type /*SelectInfo*/)
{
	OnSelectionChangedDelegate.ExecuteIfBound(GetSelectedHandles());
}

// =======================================================================================
// Row generation
// =======================================================================================

TSharedRef<ITableRow> SKzPropertyStack::OnGenerateRow(TSharedPtr<IPropertyHandle> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FText Tooltip = GetHandleToolTip(Item);

	const float HorizontalPadding = 6.0f;
	const float VerticalPadding = 3.0f;
	const FMargin Margin(HorizontalPadding, VerticalPadding);

	TSharedRef<SWidget> LeadingWidget = RowCustomizer.IsValid()
		? RowCustomizer->BuildLeadingWidget(Item) : SNullWidget::NullWidget;
	TSharedRef<SWidget> TrailingWidget = RowCustomizer.IsValid()
		? RowCustomizer->BuildTrailingWidget(Item) : SNullWidget::NullWidget;

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
						const bool bIsSelected = ListViewWidget.IsValid() && ListViewWidget->IsItemSelected(Item);

						if (RowCustomizer.IsValid())
						{
							if (const FSlateBrush* CustomBrush = RowCustomizer->GetBackgroundBrush(Item, bIsSelected))
							{
								return CustomBrush;
							}
						}

						return bIsSelected
							? FKzLibEditorStyle::Get().GetBrush("Kz.CardBorderSelected")
							: FKzLibEditorStyle::Get().GetBrush("Kz.CardBorder");
					})
				.Padding(Margin * 1.5f)
				[
					SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().MaxWidth(18).AutoWidth()
						.HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Margin)
						[
							SNew(SImage).Image(FAppStyle::GetBrush("VerticalBoxDragIndicatorShort"))
						]

						+ SHorizontalBox::Slot().AutoWidth()
						.VAlign(VAlign_Center).Padding(Margin)
						[
							LeadingWidget
						]

						+ SHorizontalBox::Slot().FillWidth(1.0f)
						.HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Margin)
						[
							SNew(STextBlock)
								.Text_Lambda([this, Item]() { return FText::FromString(GetHandleDisplayName(Item)); })
								.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
								.HighlightText(this, &SKzPropertyStack::GetSearchText)
						]

						+ SHorizontalBox::Slot().AutoWidth()
						.VAlign(VAlign_Center).Padding(Margin)
						[
							TrailingWidget
						]

						+ SHorizontalBox::Slot().AutoWidth()
						.HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(Margin)
						[
							SNew(SButton)
								.ContentPadding(FMargin(0, 2))
								.ToolTipText(FText::Format(INVTEXT("Delete this {0}."), ItemName))
								.OnClicked(this, &SKzPropertyStack::OnDeleteElementClicked, Item)
								[
									SNew(SImage)
										.Image(FAppStyle::GetBrush("Icons.Delete"))
										.ColorAndOpacity(FSlateColor::UseForeground())
								]
						]
				]
		];
}

// =======================================================================================
// Context menu
// =======================================================================================

TSharedPtr<SWidget> SKzPropertyStack::GetContextMenuContent()
{
	if (!CanCopyElements() && !CanPasteElement() && !CanDuplicateElements()) { return nullptr; }

	FMenuBuilder MenuBuilder(true, CommandList);

	MenuBuilder.BeginSection("ElementActions", INVTEXT("Actions"));
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

// =======================================================================================
// Add / delete (single-element entry points used by buttons)
// =======================================================================================

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
	if (!ArrayHandle.IsValid()) { return FReply::Handled(); }

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Add {0}"), ItemName));
	ArrayHandle->AddItem();
	RefreshStack();

	if (AllHandles.Num() > 0 && ListViewWidget.IsValid())
	{
		ListViewWidget->ClearSelection();
		ListViewWidget->SetSelection(AllHandles.Last());
		OnSelectionChangedDelegate.ExecuteIfBound({ AllHandles.Last() });
	}
	return FReply::Handled();
}

void SKzPropertyStack::OnAddObjectClassSelected(UClass* ObjectClass)
{
	if (!ArrayHandle.IsValid() || !ObjectClass || !RootHandle.IsValid()) { return; }

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Add {0}"), ItemName));

	TArray<UObject*> OuterObjects;
	RootHandle->GetOuterObjects(OuterObjects);
	UObject* OuterAsset = OuterObjects.Num() > 0 ? OuterObjects[0] : GetTransientPackage();

	if (OuterAsset) { OuterAsset->Modify(); }

	ArrayHandle->AddItem();

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	if (NumElements == 0) { return; }

	TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);
	if (NewElementHandle.IsValid())
	{
		UObject* NewObj = NewObject<UObject>(OuterAsset, ObjectClass, NAME_None, RF_Transactional);
		NewElementHandle->SetValue(NewObj);

		// Force-write through raw data if SetValue doesn't stick (instanced object quirk).
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
			ListViewWidget->ClearSelection();
			ListViewWidget->SetSelection(NewElementHandle);
		}
		OnSelectionChangedDelegate.ExecuteIfBound({ NewElementHandle });
	}
}

FReply SKzPropertyStack::OnDeleteElementClicked(TSharedPtr<IPropertyHandle> ItemToDelete)
{
	if (ArrayHandle.IsValid() && ItemToDelete.IsValid())
	{
		const FScopedTransaction Transaction(FText::Format(INVTEXT("Delete {0}"), ItemName));
		ArrayHandle->DeleteItem(ItemToDelete->GetIndexInArray());
		RefreshStack();
		OnSelectionChangedDelegate.ExecuteIfBound({});
	}
	return FReply::Handled();
}

// =======================================================================================
// Drag & drop (multi-aware)
// =======================================================================================

FReply SKzPropertyStack::OnRowDragDetected(const FGeometry& /*MyGeometry*/, const FPointerEvent& MouseEvent, TSharedPtr<IPropertyHandle> DraggedItem)
{
	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) { return FReply::Unhandled(); }

	// If the dragged item is part of the current selection, drag the whole selection.
	// Otherwise drag just that one (and switch the selection to it).
	TArray<TSharedPtr<IPropertyHandle>> Payload;
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();

	if (Selected.Contains(DraggedItem))
	{
		Payload = Selected;
	}
	else
	{
		Payload = { DraggedItem };
		if (ListViewWidget.IsValid())
		{
			ListViewWidget->ClearSelection();
			ListViewWidget->SetSelection(DraggedItem);
		}
	}

	// Sort by current array index so the drop logic preserves relative order.
	Payload.Sort([](const TSharedPtr<IPropertyHandle>& A, const TSharedPtr<IPropertyHandle>& B)
		{
			const int32 IA = A.IsValid() ? A->GetIndexInArray() : INDEX_NONE;
			const int32 IB = B.IsValid() ? B->GetIndexInArray() : INDEX_NONE;
			return IA < IB;
		});

	return FReply::Handled().BeginDragDrop(FKzPropertyDragDropOp::New(Payload));
}

TOptional<EItemDropZone> SKzPropertyStack::OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();
	if (!DragOp.IsValid() || !TargetItem.IsValid()) { return TOptional<EItemDropZone>(); }

	// Reject if target is itself part of the dragged set (would be a no-op move).
	if (DragOp->HandlesToDrag.Contains(TargetItem)) { return TOptional<EItemDropZone>(); }
	return DropZone;
}

FReply SKzPropertyStack::OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<IPropertyHandle> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();
	if (!DragOp.IsValid() || !ArrayHandle.IsValid() || !TargetItem.IsValid())
	{
		return FReply::Unhandled();
	}

	// Resolve indices fresh; the array hasn't changed yet.
	int32 TargetIndex = TargetItem->GetIndexInArray();
	if (TargetIndex == INDEX_NONE) { return FReply::Unhandled(); }
	if (DropZone == EItemDropZone::BelowItem) { ++TargetIndex; }

	// Collect source indices, sorted ascending. We move from the lowest source first
	// while adjusting indices manually to preserve relative order.
	TArray<int32> SourceIndices;
	for (const TSharedPtr<IPropertyHandle>& Handle : DragOp->HandlesToDrag)
	{
		if (Handle.IsValid())
		{
			const int32 Idx = Handle->GetIndexInArray();
			if (Idx != INDEX_NONE) { SourceIndices.AddUnique(Idx); }
		}
	}
	SourceIndices.Sort();
	if (SourceIndices.Num() == 0) { return FReply::Unhandled(); }

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Reorder {0}"), ItemName));

	// Mark outers dirty.
	if (RootHandle.IsValid())
	{
		TArray<UObject*> OuterObjects;
		RootHandle->GetOuterObjects(OuterObjects);
		for (UObject* OuterObj : OuterObjects)
		{
			if (OuterObj) { OuterObj->Modify(); }
		}
	}

	// Move each source one by one. For every move, indices below the target shift down,
	// which we account for by tracking how many we already inserted.
	int32 InsertCursor = TargetIndex;
	for (int32 i = 0; i < SourceIndices.Num(); ++i)
	{
		int32 Source = SourceIndices[i];
		// Each previously-moved source that originated below InsertCursor pulls Source up by one.
		for (int32 j = 0; j < i; ++j)
		{
			if (SourceIndices[j] < Source) { --Source; }
		}
		int32 Dest = InsertCursor;
		if (Source < Dest) { --Dest; } // moving forward: removing the source pulls the destination up

		ArrayHandle->MoveElementTo(Source, Dest);
		++InsertCursor; // next inserted item goes after the previous one
		// Source positions of remaining items above Dest remain stable in absolute terms
		// because MoveElementTo handles the shift internally.
	}

	RefreshStack();
	return FReply::Handled();
}

// =======================================================================================
// Commands (multi-aware)
// =======================================================================================

void SKzPropertyStack::BindCommands()
{
	CommandList = MakeShared<FUICommandList>();

	CommandList->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::CopySelectedElements),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanCopyElements));

	CommandList->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::PasteElement),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanPasteElement));

	CommandList->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::CutSelectedElements),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanCutElements));

	CommandList->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::DeleteSelectedElements),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanDeleteElements));

	CommandList->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &SKzPropertyStack::DuplicateSelectedElements),
		FCanExecuteAction::CreateSP(this, &SKzPropertyStack::CanDuplicateElements));
}

bool SKzPropertyStack::CanCopyElements() const
{
	return GetSelectedHandles().Num() > 0;
}

bool SKzPropertyStack::CanCutElements() const { return CanCopyElements(); }
bool SKzPropertyStack::CanDeleteElements() const { return CanCopyElements(); }
bool SKzPropertyStack::CanDuplicateElements() const { return CanCopyElements(); }
bool SKzPropertyStack::CanPasteElement() const { return ArrayHandle.IsValid(); }

void SKzPropertyStack::CopySelectedElements()
{
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	if (Selected.Num() == 0) { return; }

	if (bIsObjectArray)
	{
		// For objects, only single-element copy via the deep exporter is currently
		// supported; multi-copy of polymorphic UObjects through clipboard would need
		// a custom container format. Fall back to copying just the primary.
		if (UObject* ObjectToCopy = nullptr; Selected.Last()->GetValue(ObjectToCopy) == FPropertyAccess::Success && ObjectToCopy)
		{
			FKzClipboardUtils::CopyObjectToClipboard(ObjectToCopy);
		}
		return;
	}

	// Structs/primitives: serialize each handle and join with a separator that
	// PasteElement understands (one entry per line, prefixed to disambiguate).
	TArray<FString> Lines;
	for (const TSharedPtr<IPropertyHandle>& Handle : Selected)
	{
		if (!Handle.IsValid()) { continue; }
		FString Serialized;
		if (Handle->GetValueAsFormattedString(Serialized) == FPropertyAccess::Success)
		{
			Lines.Add(Serialized);
		}
	}

	if (Lines.Num() > 0)
	{
		// Use a clear delimiter so paste can split. Avoid newlines (struct text may have them).
		const FString Joined = FString::Join(Lines, TEXT("\n###KZ_ELEMENT###\n"));
		FPlatformApplicationMisc::ClipboardCopy(*Joined);
	}
}

void SKzPropertyStack::PasteElement()
{
	if (!ArrayHandle.IsValid() || !RootHandle.IsValid()) { return; }

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	if (ClipboardContent.IsEmpty()) { return; }

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Paste {0}"), ItemName));

	if (bIsObjectArray && BaseObjectClass)
	{
		TArray<UObject*> OuterObjects;
		RootHandle->GetOuterObjects(OuterObjects);
		UObject* OuterAsset = OuterObjects.Num() > 0 ? OuterObjects[0] : GetTransientPackage();
		if (OuterAsset) { OuterAsset->Modify(); }

		UObject* PastedObject = FKzClipboardUtils::PasteObjectFromClipboard(OuterAsset);
		if (!PastedObject) { return; }

		if (!PastedObject->IsA(BaseObjectClass))
		{
			PastedObject->ClearFlags(RF_Transactional);
			PastedObject->MarkAsGarbage();
			return;
		}

		// Duplicate-check.
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
							FNotificationInfo Info(FText::Format(
								INVTEXT("{0} of type '{1}' already exists."),
								ItemName, PastedObject->GetClass()->GetDisplayNameText()));
							Info.ExpireDuration = 3.0f;
							Info.Image = FAppStyle::GetBrush("Icons.Warning");
							FSlateNotificationManager::Get().AddNotification(Info);

							PastedObject->ClearFlags(RF_Transactional);
							PastedObject->MarkAsGarbage();
							return;
						}
					}
				}
			}
		}

		ArrayHandle->AddItem();
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);
		if (NewElementHandle.IsValid())
		{
			NewElementHandle->SetValue(PastedObject);

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
				ListViewWidget->ClearSelection();
				ListViewWidget->SetSelection(NewElementHandle);
			}
			OnSelectionChangedDelegate.ExecuteIfBound({ NewElementHandle });
		}
		return;
	}

	// Structs / primitives: split by our marker and append each chunk.
	TArray<FString> Lines;
	const FString Delim = TEXT("\n###KZ_ELEMENT###\n");
	if (ClipboardContent.Contains(Delim))
	{
		ClipboardContent.ParseIntoArray(Lines, *Delim, /*bCullEmpty=*/false);
	}
	else
	{
		Lines.Add(ClipboardContent);
	}

	TArray<TSharedPtr<IPropertyHandle>> Inserted;
	for (const FString& Chunk : Lines)
	{
		ArrayHandle->AddItem();
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		TSharedPtr<IPropertyHandle> NewElementHandle = ArrayHandle->GetElement(NumElements - 1);
		if (NewElementHandle.IsValid())
		{
			NewElementHandle->SetValueFromFormattedString(Chunk);
			Inserted.Add(NewElementHandle);
		}
	}

	RefreshStack();
	if (ListViewWidget.IsValid() && Inserted.Num() > 0)
	{
		ListViewWidget->ClearSelection();
		for (const TSharedPtr<IPropertyHandle>& H : Inserted)
		{
			ListViewWidget->SetItemSelection(H, true);
		}
	}
	OnSelectionChangedDelegate.ExecuteIfBound(Inserted);
}

void SKzPropertyStack::CutSelectedElements()
{
	const FScopedTransaction Transaction(FText::Format(INVTEXT("Cut {0}"), ItemName));
	CopySelectedElements();
	DeleteSelectedElements();
}

void SKzPropertyStack::DeleteSelectedElements()
{
	if (!ArrayHandle.IsValid()) { return; }
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	if (Selected.Num() == 0) { return; }

	// Collect indices and delete from highest to lowest so earlier deletions don't
	// invalidate later ones.
	TArray<int32> Indices;
	Indices.Reserve(Selected.Num());
	for (const TSharedPtr<IPropertyHandle>& Handle : Selected)
	{
		if (Handle.IsValid())
		{
			const int32 Idx = Handle->GetIndexInArray();
			if (Idx != INDEX_NONE) { Indices.Add(Idx); }
		}
	}
	if (Indices.Num() == 0) { return; }

	Indices.Sort();
	Algo::Reverse(Indices);

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Delete {0}"), ItemName));
	for (int32 Idx : Indices)
	{
		ArrayHandle->DeleteItem(Idx);
	}

	RefreshStack();
	if (ListViewWidget.IsValid()) { ListViewWidget->ClearSelection(); }
	OnSelectionChangedDelegate.ExecuteIfBound({});
}

void SKzPropertyStack::DuplicateSelectedElements()
{
	if (!ArrayHandle.IsValid()) { return; }
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	if (Selected.Num() == 0) { return; }

	// Sort by ascending index so we duplicate left-to-right and the new positions are
	// well-defined as we go. ArrayHandle->DuplicateItem inserts the copy right after.
	TArray<int32> Indices;
	for (const TSharedPtr<IPropertyHandle>& Handle : Selected)
	{
		if (Handle.IsValid())
		{
			const int32 Idx = Handle->GetIndexInArray();
			if (Idx != INDEX_NONE) { Indices.Add(Idx); }
		}
	}
	if (Indices.Num() == 0) { return; }
	Indices.Sort();

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Duplicate {0}"), ItemName));

	// Duplicate from highest to lowest index — duplicates inserted after a higher
	// index don't shift the still-pending lower indices.
	Algo::Reverse(Indices);
	for (int32 Idx : Indices)
	{
		ArrayHandle->DuplicateItem(Idx);
	}

	RefreshStack();

	// New duplicates live at OriginalIndex+1, but since the array shifted as we went,
	// the cleanest way to find them is to reselect by walking AllHandles and picking
	// items whose value matches the originals at the corresponding adjusted positions.
	// For simplicity, just clear the selection and let the user re-select.
	if (ListViewWidget.IsValid()) { ListViewWidget->ClearSelection(); }
	OnSelectionChangedDelegate.ExecuteIfBound({});
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "Widgets/SKzPropertyStack.h"

#include "Widgets/KzPropertyStackRowCustomizer.h"
#include "Widgets/SKzClassCombo.h"
#include "Utils/KzEditorUtils.h"
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

#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SKzPropertyStack"

void SKzPropertyStack::Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	bAllowDuplicates = InArgs._bAllowDuplicates;
	OnSelectionChangedDelegate = InArgs._OnSelectionChanged;
	OnImmutableRowSelectedDelegate = InArgs._OnImmutableRowSelected;
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

								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth().Padding(3.0f, 3.0f)
								[
									SNew(SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.ToolTipText(this, &SKzPropertyStack::GetClearAllToolTip)
										.IsEnabled(this, &SKzPropertyStack::CanClearAll)
										.OnClicked(this, &SKzPropertyStack::OnClearAllClicked)
										.ContentPadding(FMargin(4.0f, 2.0f))
										[
											SNew(SImage)
												.Image(FAppStyle::Get().GetBrush("Icons.Delete"))
												.ColorAndOpacity(FSlateColor::UseForeground())
										]
								]
						]
				]

			+ SVerticalBox::Slot().Padding(0.0f).FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(InArgs._ListPadding)
						[
							SAssignNew(ListViewWidget, SListView<TSharedPtr<FKzStackRow>>)
								.ListItemsSource(&FilteredRows)
								.OnGenerateRow(this, &SKzPropertyStack::OnGenerateRow)
								.OnSelectionChanged(this, &SKzPropertyStack::OnListSelectionChanged)
								.OnMouseButtonDoubleClick(this, &SKzPropertyStack::OnRowDoubleClick)
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
	// Every stack (one per tab) receives this and shares the editor's selection delegate.
	// Only restore from a stack that actually had a selection, so an empty one doesn't fire
	// the delegate and wipe the panel the active stack just repopulated.
	const TArray<int32> Indices = SelectedIndices;
	RefreshStack();
	if (Indices.Num() > 0)
	{
		RestoreSelectionByIndices(Indices);
	}
}

void SKzPropertyStack::PostRedo(bool /*bSuccess*/)
{
	const TArray<int32> Indices = SelectedIndices;
	RefreshStack();
	if (Indices.Num() > 0)
	{
		RestoreSelectionByIndices(Indices);
	}
}

void SKzPropertyStack::RestoreSelectionByIndices(const TArray<int32>& Indices)
{
	if (!ListViewWidget.IsValid()) { return; }

	// ClearSelection / SetItemSelection each fire OnListSelectionChanged; guard so it doesn't
	// clobber the cache or notify mid-restore. We notify once at the end instead.
	TGuardValue<bool> Guard(bRestoringSelection, true);

	ListViewWidget->ClearSelection();

	TArray<TSharedPtr<IPropertyHandle>> RestoredHandles;
	TSharedPtr<FKzStackRow> LastRow;
	for (int32 Index : Indices)
	{
		// Editable rows are built first, in array order, so AllRows[Index] is the array element.
		if (AllRows.IsValidIndex(Index) && AllRows[Index].IsValid() && AllRows[Index]->IsEditable())
		{
			ListViewWidget->SetItemSelection(AllRows[Index], true, ESelectInfo::Direct);
			RestoredHandles.Add(AllRows[Index]->Handle);
			LastRow = AllRows[Index];
		}
	}

	if (LastRow.IsValid())
	{
		ListViewWidget->RequestScrollIntoView(LastRow);
	}

	OnSelectionChangedDelegate.ExecuteIfBound(RestoredHandles);
}

// =======================================================================================
// Refresh / filter
// =======================================================================================

void SKzPropertyStack::RefreshStack()
{
	AllRows.Empty();

	// 1. Editable rows: one per array element, in array order (first).
	if (ArrayHandle.IsValid())
	{
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		for (uint32 i = 0; i < NumElements; ++i)
		{
			AllRows.Add(MakeShared<FKzStackRow>(ArrayHandle->GetElement(i)));
		}
	}

	// 2. Injected read-only rows (e.g. inherited parent entries), appended after.
	if (ImmutableRowsProvider.IsBound())
	{
		for (const TSharedPtr<FKzStackRow>& Row : ImmutableRowsProvider.Execute())
		{
			if (Row.IsValid()) { AllRows.Add(Row); }
		}
	}

	GenerateFilteredList();
}

void SKzPropertyStack::GenerateFilteredList()
{
	FilteredRows.Empty();
	const bool bHasFilterText = !TextFilter->GetFilterText().IsEmpty();

	for (const TSharedPtr<FKzStackRow>& Row : AllRows)
	{
		if (!Row.IsValid()) { continue; }
		if (!Row->IsEditable() && !Row->Snapshot.IsValid()) { continue; } // invalid injected row

		bool bPassesFilter = true;
		if (bHasFilterText)
		{
			const FString RowNameStr = GetRowDisplayName(Row);
			bPassesFilter = TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(RowNameStr));
		}

		if (bPassesFilter) { FilteredRows.Add(Row); }
	}

	// Mark the first visible row of each named group so OnGenerateRow can draw the header.
	FText CurrentGroup;
	for (const TSharedPtr<FKzStackRow>& Row : FilteredRows)
	{
		Row->bRenderGroupHeader = false;
		if (!Row->GroupName.IsEmpty() && !Row->GroupName.EqualTo(CurrentGroup))
		{
			Row->bRenderGroupHeader = true;
			CurrentGroup = Row->GroupName;
		}
	}

	if (ListViewWidget.IsValid()) { ListViewWidget->RequestListRefresh(); }
}

FString SKzPropertyStack::GetRowDisplayName(TSharedPtr<FKzStackRow> Row) const
{
	if (!Row.IsValid()) { return TEXT("Invalid"); }
	if (Row->IsEditable()) { return GetHandleDisplayName(Row->Handle); }
	return Row->DisplayLabel.IsEmpty() ? TEXT("Inherited") : Row->DisplayLabel.ToString();
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

FReply SKzPropertyStack::OnClearAllClicked()
{
	if (!ArrayHandle.IsValid()) { return FReply::Handled(); }

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	if (NumElements == 0) { return FReply::Handled(); }

	const FText PluralName = ItemNamePlural.IsEmpty() ? FText::Format(INVTEXT("{0}s"), ItemName) : ItemNamePlural;

	const FText ConfirmText = FText::Format(
		NSLOCTEXT("KzPropertyStack", "ClearAllConfirm", "Remove all {0} {1}?"),
		FText::AsNumber(NumElements),
		PluralName);

	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(FText::Format(NSLOCTEXT("KzPropertyStack", "ClearAllTrans", "Clear all {0}"), PluralName));
	ArrayHandle->EmptyArray();

	return FReply::Handled();
}

bool SKzPropertyStack::CanClearAll() const
{
	if (!ArrayHandle.IsValid()) { return false; }
	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	return NumElements > 0 && CanRemoveRows(GetAllHandles());
}

FText SKzPropertyStack::GetClearAllToolTip() const
{
	FText Reason;
	return CanRemoveRows(GetAllHandles(), &Reason) ? NSLOCTEXT("KzPropertyStack", "ClearAllTip", "Remove all elements") : Reason;
}

TArray<TSharedPtr<IPropertyHandle>> SKzPropertyStack::GetAllHandles() const
{
	TArray<TSharedPtr<IPropertyHandle>> Handles;
	uint32 NumElements = 0;
	if (ArrayHandle.IsValid() && ArrayHandle->GetNumElements(NumElements) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumElements; ++Index)
		{
			Handles.Add(ArrayHandle->GetElement(Index));
		}
	}
	return Handles;
}

bool SKzPropertyStack::CanRemoveRows(const TArray<TSharedPtr<IPropertyHandle>>& Handles, FText* OutReason) const
{
	FText Reason;
	const bool bCanRemove = !RowCustomizer.IsValid() || RowCustomizer->CanRemoveRows(Handles, Reason);
	if (OutReason) { *OutReason = Reason; }
	return bCanRemove;
}

void SKzPropertyStack::NotifyRemovalRefused(const FText& Reason) const
{
	FNotificationInfo Info(Reason);
	Info.ExpireDuration = 4.0f;
	Info.Image = FAppStyle::GetBrush("Icons.Warning");
	FSlateNotificationManager::Get().AddNotification(Info);
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

void SKzPropertyStack::SetImmutableRowsProvider(FKzImmutableRowsProvider InProvider)
{
	ImmutableRowsProvider = InProvider;
	RefreshStack();
}

void SKzPropertyStack::RefreshRows()
{
	RefreshStack();
}

// =======================================================================================
// Selection
// =======================================================================================

TArray<TSharedPtr<FKzStackRow>> SKzPropertyStack::GetSelectedRows() const
{
	if (!ListViewWidget.IsValid()) { return {}; }
	return ListViewWidget->GetSelectedItems();
}

TArray<TSharedPtr<IPropertyHandle>> SKzPropertyStack::GetSelectedHandles() const
{
	TArray<TSharedPtr<IPropertyHandle>> Handles;
	for (const TSharedPtr<FKzStackRow>& Row : GetSelectedRows())
	{
		if (Row.IsValid() && Row->IsEditable())
		{
			Handles.Add(Row->Handle);
		}
	}
	return Handles;
}

TSharedPtr<IPropertyHandle> SKzPropertyStack::GetPrimarySelectedHandle() const
{
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	return Selected.Num() > 0 ? Selected.Last() : nullptr;
}

void SKzPropertyStack::NotifySelectionChanged()
{
	const TArray<TSharedPtr<FKzStackRow>> SelectedRows = GetSelectedRows();
	const TSharedPtr<FKzStackRow> Primary = SelectedRows.Num() > 0 ? SelectedRows.Last() : nullptr;

	// A read-only row is the primary selection: surface it to the immutable handler.
	if (Primary.IsValid() && !Primary->IsEditable())
	{
		OnImmutableRowSelectedDelegate.ExecuteIfBound(Primary);
		return;
	}

	OnSelectionChangedDelegate.ExecuteIfBound(GetSelectedHandles());
}

bool SKzPropertyStack::SelectByIndex(int32 Index)
{
	if (!ListViewWidget.IsValid() || !AllRows.IsValidIndex(Index) || !AllRows[Index]->IsEditable()) { return false; }

	TSharedPtr<FKzStackRow> Target = AllRows[Index];
	ListViewWidget->ClearSelection();
	ListViewWidget->SetSelection(Target);
	ListViewWidget->RequestScrollIntoView(Target);
	OnSelectionChangedDelegate.ExecuteIfBound({ Target->Handle });
	return true;
}

bool SKzPropertyStack::SelectByContextId(const FGuid& ContextId)
{
	if (!RowCustomizer.IsValid() || !ListViewWidget.IsValid()) { return false; }

	// Gather editable handles for the customizer to resolve against.
	TArray<TSharedPtr<IPropertyHandle>> Handles;
	for (const TSharedPtr<FKzStackRow>& Row : AllRows)
	{
		if (Row.IsValid() && Row->IsEditable()) { Handles.Add(Row->Handle); }
	}

	TSharedPtr<IPropertyHandle> Resolved;
	if (!RowCustomizer->TryResolveContextId(ContextId, Handles, Resolved)) { return false; }
	if (!Resolved.IsValid()) { return false; }

	const TSharedPtr<FKzStackRow>* TargetRow = AllRows.FindByPredicate([&Resolved](const TSharedPtr<FKzStackRow>& Row)
		{
			return Row.IsValid() && Row->Handle == Resolved;
		});
	if (!TargetRow) { return false; }

	ListViewWidget->ClearSelection();
	ListViewWidget->SetSelection(*TargetRow);
	ListViewWidget->RequestScrollIntoView(*TargetRow);
	OnSelectionChangedDelegate.ExecuteIfBound({ Resolved });
	return true;
}

void SKzPropertyStack::OnListSelectionChanged(TSharedPtr<FKzStackRow> /*SelectedItem*/, ESelectInfo::Type SelectInfo)
{
	if (bRestoringSelection) { return; }

	// Only cache user-driven changes. Direct notifications (list refreshes) can carry a
	// transient empty selection that would otherwise wipe what we need to restore.
	if (SelectInfo != ESelectInfo::Direct)
	{
		SelectedIndices.Reset();
		for (const TSharedPtr<FKzStackRow>& Row : GetSelectedRows())
		{
			if (Row.IsValid() && Row->IsEditable())
			{
				const int32 Index = Row->Handle->GetIndexInArray();
				if (Index != INDEX_NONE) { SelectedIndices.Add(Index); }
			}
		}
	}

	NotifySelectionChanged();
}

// =======================================================================================
// Row generation
// =======================================================================================

void SKzPropertyStack::OnRowDoubleClick(TSharedPtr<FKzStackRow> Item)
{
	if (RowCustomizer.IsValid() && Item.IsValid() && Item->IsEditable())
	{
		RowCustomizer->OnRowDoubleClicked(Item->Handle);
	}
}

TSharedRef<ITableRow> SKzPropertyStack::OnGenerateRow(TSharedPtr<FKzStackRow> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bEditable = Item.IsValid() && Item->IsEditable();
	TSharedPtr<IPropertyHandle> Handle = Item.IsValid() ? Item->Handle : nullptr;

	const FMargin Margin(6.0f, 3.0f);

	TSharedRef<SWidget> LeadingWidget = (bEditable && RowCustomizer.IsValid())
		? RowCustomizer->BuildLeadingWidget(Handle) : SNullWidget::NullWidget;
	TSharedRef<SWidget> TrailingWidget = (bEditable && RowCustomizer.IsValid())
		? RowCustomizer->BuildTrailingWidget(Handle) : SNullWidget::NullWidget;

	const FText Tooltip = bEditable ? GetHandleToolTip(Handle) : Item->SourceTag;
	const FSlateColor TextColor = (Item.IsValid() && Item->bIsOverridden)
		? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground();

	// --- Card content ---
	TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

	// Drag handle (editable only).
	if (bEditable)
	{
		RowBox->AddSlot().MaxWidth(18).AutoWidth().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Margin)
			[
				SNew(SImage).Image(FAppStyle::GetBrush("VerticalBoxDragIndicatorShort"))
			];
	}
	else
	{
		// Lock glyph marks the row as inherited / read-only.
		RowBox->AddSlot().MaxWidth(18).AutoWidth().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Margin)
			[
				SNew(SImage).Image(FAppStyle::GetBrush("Icons.Lock")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	RowBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(Margin)
		[
			LeadingWidget
		];

	RowBox->AddSlot().FillWidth(1.0f).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(Margin)
		[
			SNew(STextBlock)
				.Text_Lambda([this, Item]() { return FText::FromString(GetRowDisplayName(Item)); })
				.ColorAndOpacity(TextColor)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.HighlightText(this, &SKzPropertyStack::GetSearchText)
		];

	// Optional origin tag (any row).
	if (Item.IsValid() && !Item->SourceTag.IsEmpty())
	{
		RowBox->AddSlot().AutoWidth().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(Margin)
			[
				SNew(STextBlock)
					.Text(FText::Format(INVTEXT("[{0}]"), Item->SourceTag))
					.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
					.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	RowBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(Margin)
		[
			TrailingWidget
		];

	// Delete button: collapsed for read-only rows so nothing reserves space to the right of the
	// source tag. Uniform row height is kept by the MinDesiredHeight box wrapping the row content.
	RowBox->AddSlot().AutoWidth().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(Margin)
		[
			SNew(SButton)
				.ContentPadding(FMargin(0, 2))
				.Visibility(bEditable ? EVisibility::Visible : EVisibility::Collapsed)
				.IsEnabled_Lambda([this, Handle, bEditable]() { return bEditable && CanRemoveRows({ Handle }); })
				.ToolTipText_Lambda([this, Handle, bEditable]()
					{
						FText Reason;
						return !bEditable || CanRemoveRows({ Handle }, &Reason) ? FText::Format(INVTEXT("Delete this {0}."), ItemName) : Reason;
					})
				.OnClicked(this, &SKzPropertyStack::OnDeleteElementClicked, Item)
				[
					SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Delete"))
						.ColorAndOpacity(FSlateColor::UseForeground())
				]
		];

	TSharedRef<SWidget> Card = SNew(SBorder)
		.ToolTipText(Tooltip)
		.BorderImage_Lambda([this, Item, bEditable]()
			{
				const bool bIsSelected = ListViewWidget.IsValid() && ListViewWidget->IsItemSelected(Item);

				if (bEditable && RowCustomizer.IsValid())
				{
					if (const FSlateBrush* CustomBrush = RowCustomizer->GetBackgroundBrush(Item->Handle, bIsSelected))
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
			SNew(SBox)
				.MinDesiredHeight(30.0f)
				.VAlign(VAlign_Center)
				[
					RowBox
				]
		];

	// --- Optional group header above the card ---
	TSharedRef<SWidget> Content = Card;
	if (Item.IsValid() && Item->bRenderGroupHeader && !Item->GroupName.IsEmpty())
	{
		Content = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(2.0f, 8.0f, 2.0f, 2.0f))
			[
				SNew(STextBlock)
					.Text(Item->GroupName)
					.Font(FAppStyle::Get().GetFontStyle("DetailsView.CategoryFontStyle"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Card
			];
	}

	return SNew(STableRow<TSharedPtr<FKzStackRow>>, OwnerTable)
		.ShowSelection(false)
		.Padding(Margin)
		.OnDragDetected(this, &SKzPropertyStack::OnRowDragDetected, Item)
		.OnCanAcceptDrop(this, &SKzPropertyStack::OnCanAcceptDrop)
		.OnAcceptDrop(this, &SKzPropertyStack::OnAcceptDrop)
		[
			Content
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

	if (AllRows.Num() > 0 && ListViewWidget.IsValid())
	{
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		if (NumElements > 0 && AllRows.IsValidIndex((int32)NumElements - 1))
		{
			TSharedPtr<FKzStackRow> NewRow = AllRows[(int32)NumElements - 1];
			ListViewWidget->ClearSelection();
			ListViewWidget->SetSelection(NewRow);
			OnSelectionChangedDelegate.ExecuteIfBound({ NewRow->Handle });
		}
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

		if (ListViewWidget.IsValid() && AllRows.IsValidIndex((int32)NumElements - 1))
		{
			TSharedPtr<FKzStackRow> NewRow = AllRows[(int32)NumElements - 1];
			ListViewWidget->ClearSelection();
			ListViewWidget->SetSelection(NewRow);
			OnSelectionChangedDelegate.ExecuteIfBound({ NewRow->Handle });
		}
	}
}

FReply SKzPropertyStack::OnDeleteElementClicked(TSharedPtr<FKzStackRow> ItemToDelete)
{
	if (ArrayHandle.IsValid() && ItemToDelete.IsValid() && ItemToDelete->IsEditable() && CanRemoveRows({ ItemToDelete->Handle }))
	{
		const FScopedTransaction Transaction(FText::Format(INVTEXT("Delete {0}"), ItemName));
		ArrayHandle->DeleteItem(ItemToDelete->Handle->GetIndexInArray());
		RefreshStack();
		OnSelectionChangedDelegate.ExecuteIfBound({});
	}
	return FReply::Handled();
}

// =======================================================================================
// Drag & drop (multi-aware, editable rows only)
// =======================================================================================

FReply SKzPropertyStack::OnRowDragDetected(const FGeometry& /*MyGeometry*/, const FPointerEvent& MouseEvent, TSharedPtr<FKzStackRow> DraggedItem)
{
	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) { return FReply::Unhandled(); }
	if (!DraggedItem.IsValid() || !DraggedItem->IsEditable()) { return FReply::Unhandled(); }

	// If the dragged item is part of the current selection, drag the whole selection.
	// Otherwise drag just that one (and switch the selection to it).
	TArray<TSharedPtr<IPropertyHandle>> Payload;
	const TArray<TSharedPtr<FKzStackRow>> SelectedRows = GetSelectedRows();

	if (SelectedRows.Contains(DraggedItem))
	{
		for (const TSharedPtr<FKzStackRow>& Row : SelectedRows)
		{
			if (Row.IsValid() && Row->IsEditable()) { Payload.Add(Row->Handle); }
		}
	}
	else
	{
		Payload = { DraggedItem->Handle };
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

TOptional<EItemDropZone> SKzPropertyStack::OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FKzStackRow> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();
	if (!DragOp.IsValid() || !TargetItem.IsValid() || !TargetItem->IsEditable()) { return TOptional<EItemDropZone>(); }

	// Reject if target is itself part of the dragged set (would be a no-op move).
	if (DragOp->HandlesToDrag.Contains(TargetItem->Handle)) { return TOptional<EItemDropZone>(); }
	return DropZone;
}

FReply SKzPropertyStack::OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FKzStackRow> TargetItem)
{
	TSharedPtr<FKzPropertyDragDropOp> DragOp = DragDropEvent.GetOperationAs<FKzPropertyDragDropOp>();
	if (!DragOp.IsValid() || !ArrayHandle.IsValid() || !TargetItem.IsValid() || !TargetItem->IsEditable())
	{
		return FReply::Unhandled();
	}

	// Resolve indices fresh; the array hasn't changed yet.
	int32 TargetIndex = TargetItem->Handle->GetIndexInArray();
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
	}

	RefreshStack();
	return FReply::Handled();
}

// =======================================================================================
// Commands (multi-aware, editable rows only via GetSelectedHandles)
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
	// Any selected row can be copied, including read-only inherited rows.
	return GetSelectedRows().Num() > 0;
}

bool SKzPropertyStack::CanCutElements() const { return GetSelectedHandles().Num() > 0; }
bool SKzPropertyStack::CanDeleteElements() const { return GetSelectedHandles().Num() > 0; }
bool SKzPropertyStack::CanDuplicateElements() const { return GetSelectedHandles().Num() > 0; }
bool SKzPropertyStack::CanPasteElement() const { return ArrayHandle.IsValid(); }

void SKzPropertyStack::CopySelectedElements()
{
	const TArray<TSharedPtr<FKzStackRow>> SelectedRows = GetSelectedRows();
	if (SelectedRows.Num() == 0) { return; }

	if (bIsObjectArray)
	{
		// For objects, only single-element copy via the deep exporter is currently supported.
		// Copy the primary editable object (read-only object rows aren't a supported case).
		for (int32 i = SelectedRows.Num() - 1; i >= 0; --i)
		{
			if (SelectedRows[i].IsValid() && SelectedRows[i]->IsEditable())
			{
				UObject* ObjectToCopy = nullptr;
				if (SelectedRows[i]->Handle->GetValue(ObjectToCopy) == FPropertyAccess::Success && ObjectToCopy)
				{
					FKzClipboardUtils::CopyObjectToClipboard(ObjectToCopy);
				}
				break;
			}
		}
		return;
	}

	// Struct/primitive arrays: serialize each selected row and join with a separator that
	// PasteElement understands. Editable rows export via their handle; read-only rows export
	// their snapshot through the array's inner struct property (so paste can import it).
	FStructProperty* InnerStructProp = nullptr;
	if (RootHandle.IsValid())
	{
		if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(RootHandle->GetProperty()))
		{
			InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
		}
	}

	TArray<FString> Lines;
	for (const TSharedPtr<FKzStackRow>& Row : SelectedRows)
	{
		if (!Row.IsValid()) { continue; }

		FString Serialized;
		if (Row->IsEditable())
		{
			if (Row->Handle->GetValueAsFormattedString(Serialized) == FPropertyAccess::Success)
			{
				Lines.Add(Serialized);
			}
		}
		else if (Row->Snapshot.IsValid() && Row->Snapshot->IsValid() && InnerStructProp && Row->Snapshot->GetStruct() == InnerStructProp->Struct)
		{
			InnerStructProp->ExportTextItem_Direct(Serialized, Row->Snapshot->GetStructMemory(), nullptr, nullptr, PPF_None);
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
			if (ListViewWidget.IsValid() && AllRows.IsValidIndex((int32)NumElements - 1))
			{
				ListViewWidget->ClearSelection();
				ListViewWidget->SetSelection(AllRows[(int32)NumElements - 1]);
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
		for (const TSharedPtr<FKzStackRow>& Row : AllRows)
		{
			if (Row.IsValid() && Row->IsEditable() && Inserted.Contains(Row->Handle))
			{
				ListViewWidget->SetItemSelection(Row, true);
			}
		}
	}
	OnSelectionChangedDelegate.ExecuteIfBound(Inserted);
}

void SKzPropertyStack::CutSelectedElements()
{
	// Refused before copying, so a cut that cannot remove leaves the clipboard as it was.
	FText Reason;
	if (!CanRemoveRows(GetSelectedHandles(), &Reason))
	{
		NotifyRemovalRefused(Reason);
		return;
	}

	const FScopedTransaction Transaction(FText::Format(INVTEXT("Cut {0}"), ItemName));
	CopySelectedElements();
	DeleteSelectedElements();
}

void SKzPropertyStack::DeleteSelectedElements()
{
	if (!ArrayHandle.IsValid()) { return; }
	const TArray<TSharedPtr<IPropertyHandle>> Selected = GetSelectedHandles();
	if (Selected.Num() == 0) { return; }

	// The command stays available so the key press is answered with why it did nothing.
	FText Reason;
	if (!CanRemoveRows(Selected, &Reason))
	{
		NotifyRemovalRefused(Reason);
		return;
	}

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

	// Finding the exact new duplicates after the array shifted is fiddly; clear the
	// selection and let the user re-select.
	if (ListViewWidget.IsValid()) { ListViewWidget->ClearSelection(); }
	OnSelectionChangedDelegate.ExecuteIfBound({});
}

#undef LOCTEXT_NAMESPACE

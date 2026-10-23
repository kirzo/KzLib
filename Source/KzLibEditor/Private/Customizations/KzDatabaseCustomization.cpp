// Copyright 2026 kirzo

#include "Customizations/KzDatabaseCustomization.h"
#include "Core/KzDatabase.h"
#include "Customizations/KzVariantCustomization.h"
#include "Validation/KzDatabaseValidation.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "SGameplayTagContainerCombo.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzDatabaseCustomization"

// -------------------------------------------------------------------------
// FKzDatabaseItemCustomization
// -------------------------------------------------------------------------

namespace KzDatabaseCustomization
{
	/** The row's issues in its table, the worst first. */
	TArray<FKzValidationIssue> GetRowIssues(const TSharedRef<IPropertyHandle>& ItemHandle)
	{
		TArray<FKzValidationIssue> RowIssues;

		// The row's widgets can outlive its handle for a frame while the panel rebuilds after rows are added or removed.
		if (!ItemHandle->IsValidHandle())
		{
			return RowIssues;
		}

		const TSharedPtr<IPropertyHandle> ItemsHandle = ItemHandle->GetParentHandle();
		const TSharedPtr<IPropertyHandle> TableHandle = ItemsHandle.IsValid() && ItemsHandle->IsValidHandle() ? ItemsHandle->GetParentHandle() : nullptr;
		TArray<void*> RawData;
		if (TableHandle.IsValid() && TableHandle->IsValidHandle())
		{
			TableHandle->AccessRawData(RawData);
		}
		if (RawData.Num() != 1 || !RawData[0])
		{
			return RowIssues;
		}

		TArray<FKzValidationIssue> Issues;
		KzDatabaseValidation::ValidateTable(*static_cast<const FKzDatabase*>(RawData[0]), NAME_None, Issues);
		const int32 Index = ItemHandle->GetIndexInArray();
		RowIssues = Issues.FilterByPredicate([Index](const FKzValidationIssue& Issue) { return Issue.ContextIndex == Index; });
		RowIssues.StableSort([](const FKzValidationIssue& A, const FKzValidationIssue& B) { return A.Severity > B.Severity; });
		return RowIssues;
	}
}

void FKzDatabaseItemCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> IDHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, ID));
	const TSharedPtr<IPropertyHandle> TagsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, Tags));
	const TSharedPtr<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, Value));
	if (!IDHandle.IsValid() || !TagsHandle.IsValid() || !ValueHandle.IsValid())
	{
		return;
	}

	const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SImage)
			.Visibility_Lambda([PropertyHandle]() { return KzDatabaseCustomization::GetRowIssues(PropertyHandle).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			.Image_Lambda([PropertyHandle]() -> const FSlateBrush*
			{
				const TArray<FKzValidationIssue> Issues = KzDatabaseCustomization::GetRowIssues(PropertyHandle);
				if (Issues.IsEmpty())
				{
					return nullptr;
				}
				return FAppStyle::GetBrush(Issues[0].Severity == EKzValidationSeverity::Error ? "Icons.Error" : Issues[0].Severity == EKzValidationSeverity::Warning ? "Icons.Warning" : "Icons.Info");
			})
			.ToolTipText_Lambda([PropertyHandle]()
			{
				TArray<FString> Messages;
				for (const FKzValidationIssue& Issue : KzDatabaseCustomization::GetRowIssues(PropertyHandle))
				{
					Messages.Add(Issue.Message.ToString());
				}
				return FText::FromString(FString::Join(Messages, TEXT("\n")));
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(110.0f)
			.ToolTipText(LOCTEXT("IdTooltip", "ID, for lookups by ID; queries go by the tags."))
			[
				IDHandle->CreatePropertyValueWidget()
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			// The container's own customization builds its combo for a full row, not for CreatePropertyValueWidget.
			SNew(SGameplayTagContainerCombo)
			.PropertyHandle(TagsHandle)
		];

	FKzVariantCustomization::AddVariantRow(PropertyHandle, ValueHandle.ToSharedRef(), ChildBuilder, NameWidget, /*bDisplayThumbnail=*/false);
}

// -------------------------------------------------------------------------
// FKzDatabaseCustomization
// -------------------------------------------------------------------------

void FKzDatabaseCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	// Asset editors that list the rows in a tab of their own hide them here, with their count and add button.
	const TSharedPtr<IPropertyHandle> ItemsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabase, Items));
	const TSharedPtr<IPropertyHandleArray> Rows = ItemsHandle.IsValid() && !PropertyHandle->HasMetaData(TEXT("HideItems")) ? ItemsHandle->AsArray() : nullptr;

	const TSharedRef<SHorizontalBox> Value = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SKzTypeSelector)
				.AllowArrays(false)
				.IsEnabled(!FKzPropertyHandleUtils::HasMetaDataInHierarchy(PropertyHandle, TEXT("FixedType")))
				.ValueType_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ValueType : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ValueTypeObject : nullptr; })
				.ContainerType_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ContainerType : EPropertyBagContainerType::None; })
				.OnTypeChanged(this, &FKzDatabaseCustomization::OnTypeChanged)
		];

	if (Rows.IsValid())
	{
		Value->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(12.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text_Lambda([ItemsHandle, Rows]()
				{
					uint32 Num = 0;
					if (ItemsHandle->IsValidHandle())
					{
						Rows->GetNumElements(Num);
					}
					return FText::Format(LOCTEXT("RowCount", "{0} {0}|plural(one=row,other=rows)"), Num);
				})
			];
		Value->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateLambda([Rows]() { Rows->AddItem(); }), LOCTEXT("AddRow", "Add a row"))
			];
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			Value
		];
}

void FKzDatabaseCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ItemsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabase, Items));
	if (!ItemsHandle.IsValid()) { return; }

	// The rows go right under the table's header, without an Items level of their own.
	if (!PropertyHandle->HasMetaData(TEXT("HideItems")))
	{
		const TSharedRef<FDetailArrayBuilder> RowsBuilder = MakeShareable(new FDetailArrayBuilder(ItemsHandle.ToSharedRef(), /*InGenerateHeader=*/false, /*InDisplayResetToDefault=*/false, /*InDisplayElementNum=*/false));
		RowsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateLambda([](TSharedRef<IPropertyHandle> RowHandle, int32, IDetailChildrenBuilder& Builder)
		{
			Builder.AddProperty(RowHandle);
		}));
		ChildBuilder.AddCustomBuilder(RowsBuilder);
	}

	// Rows can be added from an asset editor's tab too, and every new one needs the table's type.
	TSharedPtr<IPropertyHandleArray> ArrayHandle = ItemsHandle->AsArray();
	if (ArrayHandle.IsValid())
	{
		ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FKzDatabaseCustomization::OnItemsArrayChanged));
	}
}

const FKzDatabase* FKzDatabaseCustomization::GetDatabase() const
{
	if (!StructHandle.IsValid()) { return nullptr; }
	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() == 0 || !RawData[0]) { return nullptr; }
	return static_cast<const FKzDatabase*>(RawData[0]);
}

void FKzDatabaseCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (!StructHandle.IsValid()) { return; }

	FScopedTransaction Transaction(LOCTEXT("SyncDatabaseType", "Sync Database Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	for (void* Ptr : RawData)
	{
		if (!Ptr) { continue; }
		FKzDatabase* DB = static_cast<FKzDatabase*>(Ptr);

		DB->Type = FKzTypeDef(NewContainerType, NewType, NewTypeObject);

		for (FKzDatabaseItem& Item : DB->Items)
		{
			Item.SyncType(DB->Type);
		}
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();
}

void FKzDatabaseCustomization::OnItemsArrayChanged()
{
	if (!StructHandle.IsValid()) { return; }

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	for (void* Ptr : RawData)
	{
		if (!Ptr) { continue; }
		FKzDatabase* DB = static_cast<FKzDatabase*>(Ptr);

		// New rows take the table's type; the rest only reset when theirs no longer matches.
		for (FKzDatabaseItem& Item : DB->Items)
		{
			Item.SyncType(DB->Type);
		}
	}
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "Customizations/KzShuffleBagCustomization.h"
#include "Containers/KzShuffleBag.h"
#include "Customizations/KzVariantCustomization.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "KzShuffleBagCustomization"

// -------------------------------------------------------------------------
// FKzShuffleBagEntryCustomization
// -------------------------------------------------------------------------

void FKzShuffleBagEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzShuffleBagEntry, Value));
	const TSharedPtr<IPropertyHandle> WeightHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzShuffleBagEntry, Weight));
	if (!ValueHandle.IsValid() || !WeightHandle.IsValid())
	{
		return;
	}

	const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
		.ToolTipText(WeightHandle->GetToolTipText())
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("Weight", "Weight"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(60.0f)
			[
				WeightHandle->CreatePropertyValueWidget()
			]
		];

	FKzVariantCustomization::AddVariantRow(PropertyHandle, ValueHandle.ToSharedRef(), ChildBuilder, NameWidget, /*bDisplayThumbnail=*/false);
}

// -------------------------------------------------------------------------
// FKzShuffleBagCustomization
// -------------------------------------------------------------------------

void FKzShuffleBagCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	const TSharedPtr<IPropertyHandle> EntriesHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzShuffleBag, Entries));
	const TSharedPtr<IPropertyHandleArray> Entries = EntriesHandle.IsValid() ? EntriesHandle->AsArray() : nullptr;

	const TSharedRef<SHorizontalBox> Value = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SKzTypeSelector)
				.AllowArrays(false)
				.IsEnabled(!FKzPropertyHandleUtils::HasMetaDataInHierarchy(PropertyHandle, TEXT("FixedType")))
				.ValueType_Lambda([this]() { const FKzShuffleBag* Bag = GetBag(); return Bag ? Bag->Type.ValueType : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() { const FKzShuffleBag* Bag = GetBag(); return Bag ? Bag->Type.ValueTypeObject : nullptr; })
				.ContainerType_Lambda([this]() { const FKzShuffleBag* Bag = GetBag(); return Bag ? Bag->Type.ContainerType : EPropertyBagContainerType::None; })
				.OnTypeChanged(this, &FKzShuffleBagCustomization::OnTypeChanged)
		];

	if (Entries.IsValid())
	{
		Value->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(12.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text_Lambda([EntriesHandle, Entries]()
				{
					uint32 Num = 0;
					if (EntriesHandle->IsValidHandle())
					{
						Entries->GetNumElements(Num);
					}
					return FText::Format(LOCTEXT("EntryCount", "{0} {0}|plural(one=entry,other=entries)"), Num);
				})
			];
		Value->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				PropertyCustomizationHelpers::MakeAddButton(FSimpleDelegate::CreateLambda([Entries]() { Entries->AddItem(); }), LOCTEXT("AddEntry", "Add an entry"))
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

void FKzShuffleBagCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (const TSharedPtr<IPropertyHandle> NoRepeatHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzShuffleBag, bNoRepeatAcrossRounds)))
	{
		ChildBuilder.AddProperty(NoRepeatHandle.ToSharedRef());
	}

	const TSharedPtr<IPropertyHandle> EntriesHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzShuffleBag, Entries));
	if (!EntriesHandle.IsValid())
	{
		return;
	}

	// The entries go right under the bag's header, without an Entries level of their own
	const TSharedRef<FDetailArrayBuilder> EntriesBuilder = MakeShareable(new FDetailArrayBuilder(EntriesHandle.ToSharedRef(), /*InGenerateHeader=*/false, /*InDisplayResetToDefault=*/false, /*InDisplayElementNum=*/false));
	EntriesBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateLambda([](TSharedRef<IPropertyHandle> EntryHandle, int32, IDetailChildrenBuilder& Builder)
	{
		Builder.AddProperty(EntryHandle);
	}));
	ChildBuilder.AddCustomBuilder(EntriesBuilder);

	if (const TSharedPtr<IPropertyHandleArray> ArrayHandle = EntriesHandle->AsArray())
	{
		ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FKzShuffleBagCustomization::OnEntriesChanged));
	}
}

const FKzShuffleBag* FKzShuffleBagCustomization::GetBag() const
{
	if (!StructHandle.IsValid())
	{
		return nullptr;
	}

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	return RawData.Num() > 0 ? static_cast<const FKzShuffleBag*>(RawData[0]) : nullptr;
}

void FKzShuffleBagCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (!StructHandle.IsValid())
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("SyncShuffleBagType", "Sync Shuffle Bag Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Data : RawData)
	{
		if (FKzShuffleBag* Bag = static_cast<FKzShuffleBag*>(Data))
		{
			Bag->Type = FKzTypeDef(NewContainerType, NewType, NewTypeObject);
			Bag->SyncTypes();
		}
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();
}

void FKzShuffleBagCustomization::OnEntriesChanged()
{
	if (!StructHandle.IsValid())
	{
		return;
	}

	// New entries take the bag's type
	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Data : RawData)
	{
		if (FKzShuffleBag* Bag = static_cast<FKzShuffleBag*>(Data))
		{
			Bag->SyncTypes();
		}
	}
}

#undef LOCTEXT_NAMESPACE
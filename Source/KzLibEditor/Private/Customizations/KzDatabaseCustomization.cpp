// Copyright 2026 kirzo

#include "Customizations/KzDatabaseCustomization.h"
#include "Core/KzDatabase.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "KzDatabaseCustomization"

// -------------------------------------------------------------------------
// FKzDatabaseItemCustomization
// -------------------------------------------------------------------------

void FKzDatabaseItemCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			PropertyHandle->CreatePropertyValueWidget()
		];
}

void FKzDatabaseItemCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> IDHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, ID));
	ChildBuilder.AddProperty(IDHandle.ToSharedRef());

	TSharedPtr<IPropertyHandle> TagsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, Tags));
	ChildBuilder.AddProperty(TagsHandle.ToSharedRef());

	TSharedPtr<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, Value));
	ChildBuilder.AddProperty(ValueHandle.ToSharedRef()).ShouldAutoExpand(true);
}

// -------------------------------------------------------------------------
// FKzDatabaseCustomization
// -------------------------------------------------------------------------

void FKzDatabaseCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SKzTypeSelector)
				.AllowArrays(false)
				.IsEnabled(!FKzPropertyHandleUtils::HasMetaDataInHierarchy(PropertyHandle, TEXT("FixedType")))
				.ValueType_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ValueType : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ValueTypeObject : nullptr; })
				.ContainerType_Lambda([this]() { const FKzDatabase* DB = GetDatabase(); return DB ? DB->Type.ContainerType : EPropertyBagContainerType::None; })
				.OnTypeChanged(this, &FKzDatabaseCustomization::OnTypeChanged)
		];
}

void FKzDatabaseCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ItemsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabase, Items));
	if (ItemsHandle.IsValid())
	{
		ChildBuilder.AddProperty(ItemsHandle.ToSharedRef());

		TSharedPtr<IPropertyHandleArray> ArrayHandle = ItemsHandle->AsArray();
		if (ArrayHandle.IsValid())
		{
			ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FKzDatabaseCustomization::OnItemsArrayChanged));
		}
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

		// Newly added items default-construct with an invalid FKzVariant. SyncType is a no-op for them
		// (no type mismatch yet), so we need to be explicit: if there's no value, seed nothing — the
		// Value will start invalid and the user fills it via the editor.
		// For existing items, SyncType only resets if the type really doesn't match.
		for (FKzDatabaseItem& Item : DB->Items)
		{
			Item.SyncType(DB->Type);
		}
	}
}

#undef LOCTEXT_NAMESPACE
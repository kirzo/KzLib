// Copyright 2026 kirzo

#include "Customizations/KzDatabaseCustomization.h"
#include "Core/KzDatabase.h"
#include "Widgets/SKzParamDefSelector.h"

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

	TSharedPtr<IPropertyHandle> DataHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabaseItem, Data));
	ChildBuilder.AddProperty(DataHandle.ToSharedRef());
}

// -------------------------------------------------------------------------
// FKzDatabaseCustomization
// -------------------------------------------------------------------------

void FKzDatabaseCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	// Check if the property has the "FixedType" metadata
	const bool bIsFixedType = PropertyHandle->HasMetaData(TEXT("FixedType"));

	TSharedPtr<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabase, Type));
	const bool bAllowArrays = !TypeHandle->HasMetaData(TEXT("NoArrays"));

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SKzParamDefSelector)
				.Value(this, &FKzDatabaseCustomization::GetTypeValue)
				.OnValueChanged(this, &FKzDatabaseCustomization::OnTypeChanged)
				.AllowArrays(bAllowArrays)
				.IsEnabled(!bIsFixedType)
		];
}

void FKzDatabaseCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ItemsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzDatabase, Items));
	if (ItemsHandle.IsValid())
	{
		// Draw the array
		ChildBuilder.AddProperty(ItemsHandle.ToSharedRef());

		// Hook into the array changes
		TSharedPtr<IPropertyHandleArray> ArrayHandle = ItemsHandle->AsArray();
		if (ArrayHandle.IsValid())
		{
			ArrayHandle->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FKzDatabaseCustomization::OnItemsArrayChanged));
		}
	}
}

FKzParamDef FKzDatabaseCustomization::GetTypeValue() const
{
	if (!StructHandle.IsValid()) return FKzParamDef();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() > 0 && RawData[0])
	{
		return static_cast<const FKzDatabase*>(RawData[0])->Type;
	}
	return FKzParamDef();
}

void FKzDatabaseCustomization::OnTypeChanged(const FKzParamDef& NewDef)
{
	if (!StructHandle.IsValid()) return;

	FScopedTransaction Transaction(LOCTEXT("SyncDatabaseType", "Sync Database Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	for (void* Ptr : RawData)
	{
		if (!Ptr) continue;
		FKzDatabase* DB = static_cast<FKzDatabase*>(Ptr);

		DB->Type = NewDef;

		for (FKzDatabaseItem& Item : DB->Items)
		{
			Item.SyncType(DB->Type);
		}
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

void FKzDatabaseCustomization::OnItemsArrayChanged()
{
	if (!StructHandle.IsValid()) return;

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	if (RawData.Num() > 0)
	{
		// We don't open a Transaction here because usually this callback 
		// happens inside an existing Transaction (created by the "Add Item" button).

		StructHandle->NotifyPreChange();

		for (void* Ptr : RawData)
		{
			if (!Ptr) continue;

			FKzDatabase* DB = static_cast<FKzDatabase*>(Ptr);

			// Iterate all items and ensure they match the DB definition.
			// This covers new items (which will be empty) and existing ones (which will just return).
			for (FKzDatabaseItem& Item : DB->Items)
			{
				// If the item is invalid (empty bag) or doesn't match, Sync it.
				Item.SyncType(DB->Type);
			}
		}

		StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

#undef LOCTEXT_NAMESPACE
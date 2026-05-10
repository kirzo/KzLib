// Copyright 2026 kirzo

#include "Customizations/KzTypeDefCustomization.h"
#include "Core/KzTypeDef.h"
#include "Widgets/SKzTypeSelector.h"

#include "DetailWidgetRow.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "KzTypeDefCustomization"

void FKzTypeDefCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
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
				.AllowArrays(!StructHandle->HasMetaData(TEXT("NoArrays")))
				.ValueType_Lambda([this]() { const FKzTypeDef* Td = GetTypeDef(); return Td ? Td->ValueType : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() { const FKzTypeDef* Td = GetTypeDef(); return Td ? Td->ValueTypeObject : nullptr; })
				.ContainerType_Lambda([this]() { const FKzTypeDef* Td = GetTypeDef(); return Td ? Td->ContainerType : EPropertyBagContainerType::None; })
				.OnTypeChanged(this, &FKzTypeDefCustomization::OnTypeChanged)
		];
}

const FKzTypeDef* FKzTypeDefCustomization::GetTypeDef() const
{
	if (!StructHandle.IsValid()) { return nullptr; }
	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() != 1 || !RawData[0]) { return nullptr; }
	return static_cast<const FKzTypeDef*>(RawData[0]);
}

void FKzTypeDefCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (!StructHandle.IsValid()) { return; }

	FScopedTransaction Transaction(LOCTEXT("ChangeTypeDef", "Change Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Ptr : RawData)
	{
		if (!Ptr) { continue; }
		FKzTypeDef* Td = static_cast<FKzTypeDef*>(Ptr);
		*Td = FKzTypeDef(NewContainerType, NewType, NewTypeObject);
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();
}

#undef LOCTEXT_NAMESPACE
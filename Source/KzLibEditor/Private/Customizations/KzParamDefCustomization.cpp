// Copyright 2026 kirzo

#include "Customizations/KzParamDefCustomization.h"
#include "Core/KzParamDef.h"
#include "Schemas/KzParamDefSchema.h"
#include "Widgets/SKzParamDefSelector.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "SPinTypeSelector.h"
#include "EdGraphSchema_K2.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FKzParamDefCustomization"

void FKzParamDefCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	TSharedPtr<IPropertyHandle> NameHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzParamDef, Name));

	if (NameHandle.IsValid())
	{
		NameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzParamDefCustomization::OnNameChanged));
	}

	const UKzParamDefSchema* Schema = GetDefault<UKzParamDefSchema>();
	
	// --- Metadata Checks ---
	const bool bHideName = StructHandle->HasMetaData(TEXT("HideName"));
	const bool bAllowArrays = !StructHandle->HasMetaData(TEXT("NoArrays"));

	TSharedRef<SHorizontalBox> ValueBox = SNew(SHorizontalBox);

	if (!bHideName)
	{
		ValueBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SBox)
					.WidthOverride(120.0f)
					[
						NameHandle->CreatePropertyValueWidget()
					]
			];
	}

	ValueBox->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SKzParamDefSelector)
				.Value(this, &FKzParamDefCustomization::GetValue)
				.OnValueChanged(this, &FKzParamDefCustomization::OnValueChanged)
				.AllowArrays(bAllowArrays)
		];

	HeaderRow
		.NameContent()
		[
			StructHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			ValueBox
		];
}

void FKzParamDefCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FKzParamDefCustomization::OnNameChanged()
{
	if (StructHandle.IsValid())
	{
		StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

FKzParamDef FKzParamDefCustomization::GetValue() const
{
	if (!StructHandle.IsValid()) return FKzParamDef();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() > 0 && RawData[0])
	{
		return *static_cast<FKzParamDef*>(RawData[0]);
	}
	return FKzParamDef();
}

void FKzParamDefCustomization::OnValueChanged(const FKzParamDef& NewDef)
{
	if (!StructHandle.IsValid()) return;

	// Transaction logic...
	FScopedTransaction Transaction(LOCTEXT("ChangeParamType", "Change Parameter Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Ptr : RawData)
	{
		if (Ptr) *static_cast<FKzParamDef*>(Ptr) = NewDef;
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

#undef LOCTEXT_NAMESPACE
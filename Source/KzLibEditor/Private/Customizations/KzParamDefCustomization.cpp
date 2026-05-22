// Copyright 2026 kirzo

#include "Customizations/KzParamDefCustomization.h"
#include "Core/KzParamDef.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
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

	// --- Metadata Checks ---
	const bool bHideName = FKzPropertyHandleUtils::HasMetaDataInHierarchy(StructHandle, TEXT("HideName"));
	const bool bAllowArrays = !FKzPropertyHandleUtils::HasMetaDataInHierarchy(StructHandle, TEXT("NoArrays"));
	const bool bIsFixedType = FKzPropertyHandleUtils::HasMetaDataInHierarchy(StructHandle, TEXT("FixedType"));

	TSharedRef<SHorizontalBox> ValueBox = SNew(SHorizontalBox);

	if (!bHideName && NameHandle.IsValid())
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
			SNew(SKzTypeSelector)
				.AllowArrays(bAllowArrays)
				.IsEnabled(!bIsFixedType)
				.ValueType_Lambda([this]() { const FKzParamDef* PD = GetParamDef(); return PD ? PD->Type.ValueType : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() -> const UObject* { const FKzParamDef* PD = GetParamDef(); return PD ? PD->Type.ValueTypeObject.Get() : nullptr; })
				.ContainerType_Lambda([this]() { const FKzParamDef* PD = GetParamDef(); return PD ? PD->Type.ContainerType : EPropertyBagContainerType::None; })
				.OnTypeChanged(this, &FKzParamDefCustomization::OnTypeChanged)
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

const FKzParamDef* FKzParamDefCustomization::GetParamDef() const
{
	if (!StructHandle.IsValid()) { return nullptr; }
	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() != 1 || !RawData[0]) { return nullptr; }
	return static_cast<const FKzParamDef*>(RawData[0]);
}

void FKzParamDefCustomization::OnNameChanged()
{
	if (StructHandle.IsValid())
	{
		StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FKzParamDefCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (!StructHandle.IsValid()) { return; }

	FScopedTransaction Transaction(LOCTEXT("ChangeParamDefType", "Change Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Ptr : RawData)
	{
		if (!Ptr) { continue; }
		FKzParamDef* PD = static_cast<FKzParamDef*>(Ptr);
		PD->Type = FKzTypeDef(NewContainerType, NewType, NewTypeObject);
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();
}

#undef LOCTEXT_NAMESPACE
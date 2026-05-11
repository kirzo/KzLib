// Copyright 2026 kirzo

#include "Customizations/KzParamDefCustomization.h"
#include "Core/KzParamDef.h"
#include "Schemas/KzParamDefSchema.h"
#include "Widgets/SKzParamDefSelector.h"
#include "Utils/KzEditorUtils.h"

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
	TSharedPtr<IPropertyHandle> TypeHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzParamDef, Type));

	if (NameHandle.IsValid())
	{
		NameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzParamDefCustomization::OnNameChanged));
	}

	// --- Metadata Checks ---
	const bool bHideName = FKzPropertyHandleUtils::HasMetaDataInHierarchy(StructHandle, TEXT("HideName"));

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
			TypeHandle->CreatePropertyValueWidget()
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

#undef LOCTEXT_NAMESPACE
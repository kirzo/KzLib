// Copyright 2026 kirzo

#include "Customizations/KzVariantCustomization.h"
#include "Core/KzVariant.h"
#include "Widgets/SKzTypeSelector.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "ScopedTransaction.h"

#include "Widgets/Layout/SBox.h"
#include "SEnumCombo.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SNullWidget.h"

#define LOCTEXT_NAMESPACE "KzVariantCustomization"

namespace
{
	const FName NAME_BoolValue				= TEXT("BoolValue");
	const FName NAME_ByteValue				= TEXT("ByteValue");
	const FName NAME_EnumValue				= TEXT("EnumValue");
	const FName NAME_Int32Value				= TEXT("Int32Value");
	const FName NAME_Int64Value				= TEXT("Int64Value");
	const FName NAME_DoubleValue			= TEXT("DoubleValue");
	const FName NAME_NameValue				= TEXT("NameValue");
	const FName NAME_StringValue			= TEXT("StringValue");
	const FName NAME_TextValue				= TEXT("TextValue");
	const FName NAME_StructValue			= TEXT("StructValue");
	const FName NAME_ObjectValue			= TEXT("ObjectValue");
	const FName NAME_SoftObjectValue	= TEXT("SoftObjectValue");
	const FName NAME_ClassValue				= TEXT("ClassValue");
	const FName NAME_SoftClassValue		= TEXT("SoftClassValue");
}

FName FKzVariantCustomization::GetSlotPropertyName(EPropertyBagPropertyType Type)
{
	switch (Type)
	{
		case EPropertyBagPropertyType::Bool:				return NAME_BoolValue;
		case EPropertyBagPropertyType::Byte:				return NAME_ByteValue;
		case EPropertyBagPropertyType::Enum:				return NAME_EnumValue;
		case EPropertyBagPropertyType::Int32:				return NAME_Int32Value;
		case EPropertyBagPropertyType::Int64:				return NAME_Int64Value;
		case EPropertyBagPropertyType::Float:				return NAME_DoubleValue;
		case EPropertyBagPropertyType::Double:			return NAME_DoubleValue;
		case EPropertyBagPropertyType::Name:				return NAME_NameValue;
		case EPropertyBagPropertyType::String:			return NAME_StringValue;
		case EPropertyBagPropertyType::Text:				return NAME_TextValue;
		case EPropertyBagPropertyType::Struct:			return NAME_StructValue;
		case EPropertyBagPropertyType::Object:			return NAME_ObjectValue;
		case EPropertyBagPropertyType::SoftObject:	return NAME_SoftObjectValue;
		case EPropertyBagPropertyType::Class:				return NAME_ClassValue;
		case EPropertyBagPropertyType::SoftClass:		return NAME_SoftClassValue;
		default:																		return NAME_None;
	}
}

void FKzVariantCustomization::RebuildEnumComboBox()
{
	if (!EnumComboHolder.IsValid() || !StructHandle.IsValid()) { return; }

	const UEnum* EnumPtr = Cast<UEnum>(GetCurrentTypeObject());
	TSharedPtr<IPropertyHandle> EnumHandle = StructHandle->GetChildHandle(NAME_EnumValue);
	if (!EnumPtr || !EnumHandle.IsValid())
	{
		EnumComboHolder->SetContent(SNullWidget::NullWidget);
		return;
	}

	EnumComboHolder->SetContent(
		SNew(SBox).HeightOverride(20)
		[
			SNew(SEnumComboBox, EnumPtr)
			.CurrentValue_Lambda([EnumHandle]() -> int32
				{
					int64 V = 0;
					EnumHandle->GetValue(V);
					return static_cast<int32>(V);
				})
			.OnEnumSelectionChanged_Lambda([EnumHandle](int32 NewValue, ESelectInfo::Type)
				{
					EnumHandle->SetValue(static_cast<int64>(NewValue));
				})
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	);
}

void FKzVariantCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SKzTypeSelector)
				.AllowArrays(false)
				.ValueType_Lambda([this]() { return GetCurrentType(); })
				.ValueTypeObject_Lambda([this]() { return GetCurrentTypeObject(); })
				.OnTypeChanged(this, &FKzVariantCustomization::OnTypeChanged)
		];
}

void FKzVariantCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Add every slot to the layout, but only the one matching the current type is visible.
	// This way, when the type changes, the new slot is already in the layout — no manual refresh needed.
	const FName SlotNames[] = {
		NAME_BoolValue, NAME_ByteValue, NAME_EnumValue, NAME_Int32Value, NAME_Int64Value,
		NAME_DoubleValue, NAME_NameValue, NAME_StringValue, NAME_TextValue,
		NAME_StructValue, NAME_ObjectValue, NAME_SoftObjectValue,
		NAME_ClassValue, NAME_SoftClassValue
	};

	for (const FName& SlotName : SlotNames)
	{
		TSharedPtr<IPropertyHandle> SlotHandle = PropertyHandle->GetChildHandle(SlotName);
		if (!SlotHandle.IsValid()) { continue; }

		TAttribute<EVisibility> SlotVisibility = TAttribute<EVisibility>::CreateLambda([this, SlotName]()
			{
				return GetSlotPropertyName(GetCurrentType()) == SlotName ? EVisibility::Visible : EVisibility::Collapsed;
			});

		if (SlotName == NAME_EnumValue)
		{
			ChildBuilder.AddCustomRow(LOCTEXT("EnumValueFilter", "Value"))
				.Visibility(SlotVisibility)
				.NameContent()
				[
					SNew(STextBlock).Text(LOCTEXT("Value", "Value")).Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.ValueContent()
				[
					SAssignNew(EnumComboHolder, SBox)
				];

			RebuildEnumComboBox();
		}
		else
		{
			IDetailPropertyRow& Row = ChildBuilder.AddProperty(SlotHandle.ToSharedRef());
			Row.Visibility(SlotVisibility);

			if (SlotName == NAME_StructValue)
			{
				SlotHandle->SetExpanded(true);
				Row.ShouldAutoExpand(true);

				// The type is already chosen by our type selector — hide FInstancedStruct's own struct picker.
				// bShowChildren=true keeps the inner struct properties (e.g. X/Y/Z for Vector) as expandable children.
				Row.CustomWidget(/*bShowChildren=*/true)
					.NameContent()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("Value", "Value"))
							.Font(IDetailLayoutBuilder::GetDetailFont())
					]
					.ValueContent()
					[
						SNullWidget::NullWidget
					]
					.Visibility(SlotVisibility);
			}
			else
			{
				Row.DisplayName(LOCTEXT("Value", "Value"));
			}
		}
	}
}

const FKzVariant* FKzVariantCustomization::GetVariant() const
{
	if (!StructHandle.IsValid()) { return nullptr; }

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() != 1 || !RawData[0]) { return nullptr; }
	return static_cast<const FKzVariant*>(RawData[0]);
}

EPropertyBagPropertyType FKzVariantCustomization::GetCurrentType() const
{
	const FKzVariant* V = GetVariant();
	return V ? V->GetType() : EPropertyBagPropertyType::None;
}

const UObject* FKzVariantCustomization::GetCurrentTypeObject() const
{
	const FKzVariant* V = GetVariant();
	return V ? V->GetTypeObject() : nullptr;
}

void FKzVariantCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType /*NewContainerType*/)
{
	if (!StructHandle.IsValid()) { return; }

	FScopedTransaction Transaction(LOCTEXT("ChangeVariantType", "Change Variant Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	for (void* Ptr : RawData)
	{
		if (!Ptr) { continue; }
		FKzVariant* Variant = static_cast<FKzVariant*>(Ptr);
		Variant->SetType(NewType, NewTypeObject);
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();
	RebuildEnumComboBox();
}

#undef LOCTEXT_NAMESPACE
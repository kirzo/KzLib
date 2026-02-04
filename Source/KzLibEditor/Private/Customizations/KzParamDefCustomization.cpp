// Copyright 2026 kirzo

#include "Customizations/KzParamDefCustomization.h"
#include "Core/KzParamDef.h"
#include "Schemas/KzParamDefSchema.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "SPinTypeSelector.h"
#include "EdGraphSchema_K2.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FKzParamDefCustomization"

namespace KzLib::Editor
{
	/** Helper to convert internal PropertyBag types to Editor Pin Types */
	FEdGraphPinType PinTypeFromDef(const FKzParamDef& InDef)
	{
		FEdGraphPinType PinType;

		// 1. Container Type
		switch (InDef.ContainerType)
		{
		case EPropertyBagContainerType::Array:
			PinType.ContainerType = EPinContainerType::Array;
			break;
		default:
			PinType.ContainerType = EPinContainerType::None;
			break;
		}

		// 2. Value Type
		switch (InDef.ValueType)
		{
		case EPropertyBagPropertyType::Bool:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			break;
		case EPropertyBagPropertyType::Byte:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
			break;
		case EPropertyBagPropertyType::Int32:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			break;
		case EPropertyBagPropertyType::Int64:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
			break;
		case EPropertyBagPropertyType::Float:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
			break;
		case EPropertyBagPropertyType::Double:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			break;
		case EPropertyBagPropertyType::Name:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
			break;
		case EPropertyBagPropertyType::String:
			PinType.PinCategory = UEdGraphSchema_K2::PC_String;
			break;
		case EPropertyBagPropertyType::Text:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
			break;
		case EPropertyBagPropertyType::Enum:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		case EPropertyBagPropertyType::Struct:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		case EPropertyBagPropertyType::Object:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		case EPropertyBagPropertyType::SoftObject:
			PinType.PinCategory = UEdGraphSchema_K2::PC_SoftObject;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		case EPropertyBagPropertyType::Class:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		case EPropertyBagPropertyType::SoftClass:
			PinType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
			PinType.PinSubCategoryObject = const_cast<UObject*>(InDef.ValueTypeObject.Get());
			break;
		}

		return PinType;
	}

	/** Helper to convert Editor Pin Types to internal PropertyBag types */
	FKzParamDef DefFromPinType(const FName& Name, const FEdGraphPinType& PinType)
	{
		FKzParamDef NewDef;
		NewDef.Name = Name;

		// 1. Container
		switch (PinType.ContainerType)
		{
		case EPinContainerType::None:
			NewDef.ContainerType = EPropertyBagContainerType::None;
			break;
		case EPinContainerType::Array:
			NewDef.ContainerType = EPropertyBagContainerType::Array;
			break;
		case EPinContainerType::Set:
			ensureMsgf(false, TEXT("Unsuported container type [Set]"));
			break;
		case EPinContainerType::Map:
			ensureMsgf(false, TEXT("Unsuported container type [Map]"));
			break;
		}

		// 2. Value Type
		const FName& Cat = PinType.PinCategory;
		const FName& SubCat = PinType.PinSubCategory;
		UObject* SubObj = PinType.PinSubCategoryObject.Get();

		if (Cat == UEdGraphSchema_K2::PC_Boolean)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Bool;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Byte)
		{
			if (Cast<UEnum>(SubObj))
			{
				NewDef.ValueType = EPropertyBagPropertyType::Enum;
				NewDef.ValueTypeObject = SubObj;
			}
			else
			{
				NewDef.ValueType = EPropertyBagPropertyType::Byte;
			}
		}
		else if (Cat == UEdGraphSchema_K2::PC_Int)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Int32;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Int64)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Int64;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Real)
		{
			if (SubCat == UEdGraphSchema_K2::PC_Double)
			{
				NewDef.ValueType = EPropertyBagPropertyType::Double;
			}
			else
			{
				NewDef.ValueType = EPropertyBagPropertyType::Float;
			}
		}
		else if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Name;
		}
		else if (Cat == UEdGraphSchema_K2::PC_String)
		{
			NewDef.ValueType = EPropertyBagPropertyType::String;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Text)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Text;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Enum)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Enum;
			NewDef.ValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Struct)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Struct;
			NewDef.ValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Object)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Object;
			NewDef.ValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_SoftObject)
		{
			NewDef.ValueType = EPropertyBagPropertyType::SoftObject;
			NewDef.ValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Class)
		{
			NewDef.ValueType = EPropertyBagPropertyType::Class;
			NewDef.ValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_SoftClass)
		{
			NewDef.ValueType = EPropertyBagPropertyType::SoftClass;
			NewDef.ValueTypeObject = SubObj;
		}
		else
		{
			ensureMsgf(false, TEXT("Unhandled pin category %s"), *PinType.PinCategory.ToString());
		}

		return NewDef;
	}
}

void FKzParamDefCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	TSharedPtr<IPropertyHandle> NameHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzParamDef, Name));

	if (NameHandle.IsValid())
	{
		NameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzParamDefCustomization::OnNameChanged));
	}

	PinTypeAttribute = TAttribute<FEdGraphPinType>::Create(TAttribute<FEdGraphPinType>::FGetter::CreateSP(this, &FKzParamDefCustomization::GetTargetPinType));

	const UKzParamDefSchema* Schema = GetDefault<UKzParamDefSchema>();
	const bool bAllowArrays = !StructHandle->HasMetaData(TEXT("NoArrays"));

	HeaderRow
		.NameContent()
		[
			StructHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)

				// Name Input
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(SBox)
						.WidthOverride(120.0f)
						[
							NameHandle->CreatePropertyValueWidget()
						]
				]

				// Type Selector
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
						.MaxDesiredHeight(20.0f)
						[
							SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UKzParamDefSchema::GetKzParamTypeTree))
								.Schema(Schema)
								.TargetPinType(PinTypeAttribute)
								.OnPinTypeChanged(this, &FKzParamDefCustomization::OnPinTypeChanged)
								.TypeTreeFilter(ETypeTreeFilter::None)
								.bAllowArrays(bAllowArrays)
								.Font(IDetailLayoutBuilder::GetDetailFont())
						]
				]
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

FEdGraphPinType FKzParamDefCustomization::GetTargetPinType() const
{
	if (!StructHandle.IsValid())
	{
		return FEdGraphPinType();
	}

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	if (RawData.Num() > 0 && RawData[0])
	{
		const FKzParamDef* Def = static_cast<const FKzParamDef*>(RawData[0]);
		return KzLib::Editor::PinTypeFromDef(*Def);
	}

	return FEdGraphPinType();
}

void FKzParamDefCustomization::OnPinTypeChanged(const FEdGraphPinType& InPinType)
{
	if (!StructHandle.IsValid()) return;

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);

	if (RawData.Num() > 0)
	{
		FScopedTransaction Transaction(LOCTEXT("ChangeParamType", "Change Parameter Type"));
		StructHandle->NotifyPreChange();

		for (void* DataPtr : RawData)
		{
			if (!DataPtr) continue;

			FKzParamDef* Def = static_cast<FKzParamDef*>(DataPtr);
			// Update the def using the helper, preserving the existing name
			FKzParamDef NewDef = KzLib::Editor::DefFromPinType(Def->Name, InPinType);

			// Apply changes
			Def->ValueType = NewDef.ValueType;
			Def->ContainerType = NewDef.ContainerType;
			Def->ValueTypeObject = NewDef.ValueTypeObject;
		}

		StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

#undef LOCTEXT_NAMESPACE
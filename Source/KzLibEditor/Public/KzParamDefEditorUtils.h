// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "Core/KzParamDef.h"
#include "EdGraphSchema_K2.h"

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
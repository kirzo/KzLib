// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "StructUtils/PropertyBag.h"

namespace KzLib::Editor
{
	/** Converts internal PropertyBag type info to an Editor Pin Type. */
	inline FEdGraphPinType PinTypeFromBagType(EPropertyBagPropertyType ValueType, const UObject* ValueTypeObject, EPropertyBagContainerType ContainerType)
	{
		FEdGraphPinType PinType;

		// 1. Container Type
		switch (ContainerType)
		{
		case EPropertyBagContainerType::Array:
			PinType.ContainerType = EPinContainerType::Array;
			break;
		default:
			PinType.ContainerType = EPinContainerType::None;
			break;
		}

		// 2. Value Type
		switch (ValueType)
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
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		case EPropertyBagPropertyType::Struct:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		case EPropertyBagPropertyType::Object:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		case EPropertyBagPropertyType::SoftObject:
			PinType.PinCategory = UEdGraphSchema_K2::PC_SoftObject;
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		case EPropertyBagPropertyType::Class:
			PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		case EPropertyBagPropertyType::SoftClass:
			PinType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
			PinType.PinSubCategoryObject = const_cast<UObject*>(ValueTypeObject);
			break;
		}

		return PinType;
	}

	/** Converts an Editor Pin Type to internal PropertyBag type info. */
	inline void BagTypeFromPinType(const FEdGraphPinType& PinType, EPropertyBagPropertyType& OutValueType, const UObject*& OutValueTypeObject, EPropertyBagContainerType& OutContainerType)
	{
		// 1. Container
		switch (PinType.ContainerType)
		{
		case EPinContainerType::None:
			OutContainerType = EPropertyBagContainerType::None;
			break;
		case EPinContainerType::Array:
			OutContainerType = EPropertyBagContainerType::Array;
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
			OutValueType = EPropertyBagPropertyType::Bool;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Byte)
		{
			if (Cast<UEnum>(SubObj))
			{
				OutValueType = EPropertyBagPropertyType::Enum;
				OutValueTypeObject = SubObj;
			}
			else
			{
				OutValueType = EPropertyBagPropertyType::Byte;
			}
		}
		else if (Cat == UEdGraphSchema_K2::PC_Int)
		{
			OutValueType = EPropertyBagPropertyType::Int32;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Int64)
		{
			OutValueType = EPropertyBagPropertyType::Int64;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Real)
		{
			if (SubCat == UEdGraphSchema_K2::PC_Double)
			{
				OutValueType = EPropertyBagPropertyType::Double;
			}
			else
			{
				OutValueType = EPropertyBagPropertyType::Float;
			}
		}
		else if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			OutValueType = EPropertyBagPropertyType::Name;
		}
		else if (Cat == UEdGraphSchema_K2::PC_String)
		{
			OutValueType = EPropertyBagPropertyType::String;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Text)
		{
			OutValueType = EPropertyBagPropertyType::Text;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Enum)
		{
			OutValueType = EPropertyBagPropertyType::Enum;
			OutValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Struct)
		{
			OutValueType = EPropertyBagPropertyType::Struct;
			OutValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Object)
		{
			OutValueType = EPropertyBagPropertyType::Object;
			OutValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_SoftObject)
		{
			OutValueType = EPropertyBagPropertyType::SoftObject;
			OutValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_Class)
		{
			OutValueType = EPropertyBagPropertyType::Class;
			OutValueTypeObject = SubObj;
		}
		else if (Cat == UEdGraphSchema_K2::PC_SoftClass)
		{
			OutValueType = EPropertyBagPropertyType::SoftClass;
			OutValueTypeObject = SubObj;
		}
		else
		{
			ensureMsgf(false, TEXT("Unhandled pin category %s"), *PinType.PinCategory.ToString());
		}
	}
}
// Copyright 2026 kirzo

#include "Core/KzTypeDef.h"
#include "UObject/TextProperty.h"

const FKzTypeDef FKzTypeDef::Invalid = FKzTypeDef();

bool FKzTypeDef::MatchesProperty(const FProperty* Property) const
{
	if (!Property) { return false; }

	// Match container kind first (array / set / map / none).
	if (ContainerType == EPropertyBagContainerType::Array)
	{
		const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property);
		if (!ArrayProp) { return false; }
		return FKzTypeDef(ValueType, ValueTypeObject).MatchesProperty(ArrayProp->Inner);
	}
	if (ContainerType != EPropertyBagContainerType::None)
	{
		// Set/Map not supported here yet.
		return false;
	}

	switch (ValueType)
	{
		case EPropertyBagPropertyType::Bool:		return Property->IsA<FBoolProperty>();
		case EPropertyBagPropertyType::Byte:		return Property->IsA<FByteProperty>();
		case EPropertyBagPropertyType::Int32:		return Property->IsA<FIntProperty>();
		case EPropertyBagPropertyType::Int64:		return Property->IsA<FInt64Property>();
		case EPropertyBagPropertyType::Float:
		case EPropertyBagPropertyType::Double:	return Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>();
		case EPropertyBagPropertyType::Name:		return Property->IsA<FNameProperty>();
		case EPropertyBagPropertyType::String:	return Property->IsA<FStrProperty>();
		case EPropertyBagPropertyType::Text:		return Property->IsA<FTextProperty>();

		case EPropertyBagPropertyType::Enum:
		{
			const UEnum* ExpectedEnum = Cast<UEnum>(ValueTypeObject);
			if (const FEnumProperty* EP = CastField<FEnumProperty>(Property)) { return EP->GetEnum() == ExpectedEnum; }
			if (const FByteProperty* BP = CastField<FByteProperty>(Property)) { return BP->Enum == ExpectedEnum; }
			return false;
		}
		case EPropertyBagPropertyType::Struct:
		{
			const FStructProperty* SP = CastField<FStructProperty>(Property);
			return SP && SP->Struct == Cast<UScriptStruct>(ValueTypeObject);
		}
		case EPropertyBagPropertyType::Object:
		{
			const FObjectProperty* OP = CastField<FObjectProperty>(Property);
			const UClass* ExpectedClass = Cast<UClass>(ValueTypeObject);
			return OP && (!ExpectedClass || OP->PropertyClass->IsChildOf(ExpectedClass));
		}
		case EPropertyBagPropertyType::SoftObject:
		{
			const FSoftObjectProperty* OP = CastField<FSoftObjectProperty>(Property);
			const UClass* ExpectedClass = Cast<UClass>(ValueTypeObject);
			return OP && (!ExpectedClass || OP->PropertyClass->IsChildOf(ExpectedClass));
		}
		case EPropertyBagPropertyType::Class:
		{
			const FClassProperty* CP = CastField<FClassProperty>(Property);
			const UClass* ExpectedClass = Cast<UClass>(ValueTypeObject);
			return CP && (!ExpectedClass || CP->MetaClass->IsChildOf(ExpectedClass));
		}
		case EPropertyBagPropertyType::SoftClass:
		{
			const FSoftClassProperty* CP = CastField<FSoftClassProperty>(Property);
			const UClass* ExpectedClass = Cast<UClass>(ValueTypeObject);
			return CP && (!ExpectedClass || CP->MetaClass->IsChildOf(ExpectedClass));
		}
		default: return false;
	}
}
// Copyright 2026 kirzo

#include "Core/KzVariant.h"

const FKzVariant FKzVariant::Invalid = FKzVariant();

void FKzVariant::Reset()
{
	Type = EPropertyBagPropertyType::None;
	BoolValue = false;
	ByteValue = 0;
	EnumType = nullptr;
	EnumValue = 0;
	Int32Value = 0;
	Int64Value = 0;
	DoubleValue = 0.0;
	NameValue = NAME_None;
	StringValue.Reset();
	TextValue = FText::GetEmpty();
	StructValue.Reset();
	ObjectValue = nullptr;
	SoftObjectValue.Reset();
	ClassValue = nullptr;
	SoftClassValue.Reset();
}

void FKzVariant::SetType(EPropertyBagPropertyType NewType, const UObject* NewTypeObject)
{
	Reset();
	Type = NewType;

	// For Struct, we need to seed the FInstancedStruct with the right UScriptStruct
	// so that the editor's struct picker doesn't show "None" until the user picks one.
	if (NewType == EPropertyBagPropertyType::Struct)
	{
		if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(NewTypeObject))
		{
			StructValue.InitializeAs(ScriptStruct);
		}
	}
	// For Class slot, similarly seed the class so the picker reflects the chosen UClass.
	else if (NewType == EPropertyBagPropertyType::Class)
	{
		if (const UClass* ChosenClass = Cast<UClass>(NewTypeObject))
		{
			ClassValue = const_cast<UClass*>(ChosenClass);
		}
	}
	else if (NewType == EPropertyBagPropertyType::Enum)
	{
		EnumType = const_cast<UEnum*>(Cast<UEnum>(NewTypeObject));
		if (EnumType && EnumType->NumEnums() > 1)
		{
			// Seed to the first valid entry so the dropdown shows a meaningful value rather than an empty/numeric fallback.
			EnumValue = EnumType->GetValueByIndex(0);
		}
	}
}

const UObject* FKzVariant::GetTypeObject() const
{
	switch (Type)
	{
		case EPropertyBagPropertyType::Struct:			return StructValue.GetScriptStruct();
		case EPropertyBagPropertyType::Object:			return ObjectValue ? ObjectValue->GetClass() : nullptr;
		case EPropertyBagPropertyType::Class:				return ClassValue;
		case EPropertyBagPropertyType::SoftObject:	return nullptr; // soft refs don't carry type without resolving
		case EPropertyBagPropertyType::SoftClass:		return nullptr;
		case EPropertyBagPropertyType::Enum:				return EnumType;
		default:																		return nullptr;
	}
}

void* FKzVariant::GetData()
{
	switch (Type)
	{
		case EPropertyBagPropertyType::Bool:				return &BoolValue;
		case EPropertyBagPropertyType::Byte:				return &ByteValue;
		case EPropertyBagPropertyType::Enum:				return &EnumValue;
		case EPropertyBagPropertyType::Int32:				return &Int32Value;
		case EPropertyBagPropertyType::Int64:				return &Int64Value;
		case EPropertyBagPropertyType::Float:				return &DoubleValue;
		case EPropertyBagPropertyType::Double:			return &DoubleValue;
		case EPropertyBagPropertyType::Name:				return &NameValue;
		case EPropertyBagPropertyType::String:			return &StringValue;
		case EPropertyBagPropertyType::Text:				return &TextValue;
		case EPropertyBagPropertyType::Struct:			return StructValue.GetMutableMemory();
		case EPropertyBagPropertyType::Object:			return &ObjectValue;
		case EPropertyBagPropertyType::SoftObject:	return &SoftObjectValue;
		case EPropertyBagPropertyType::Class:				return &ClassValue;
		case EPropertyBagPropertyType::SoftClass:		return &SoftClassValue;
		default:																		return nullptr;
	}
}

const void* FKzVariant::GetData() const
{
	return const_cast<FKzVariant*>(this)->GetData();
}

FKzVariant FKzVariant::FromProperty(const FProperty* Property, const void* ValuePtr)
{
	if (!Property || !ValuePtr) return FKzVariant();

	if (const FBoolProperty*		P = CastField<FBoolProperty>(Property))		{ return Make<bool>(P->GetPropertyValue(ValuePtr)); }
	if (const FByteProperty*		P = CastField<FByteProperty>(Property))		{ return Make<uint8>(P->GetPropertyValue(ValuePtr)); }
	if (const FIntProperty*			P = CastField<FIntProperty>(Property))		{ return Make<int32>(P->GetPropertyValue(ValuePtr)); }
	if (const FInt64Property*		P = CastField<FInt64Property>(Property))	{ return Make<int64>(P->GetPropertyValue(ValuePtr)); }
	if (const FFloatProperty*		P = CastField<FFloatProperty>(Property))	{ return Make<double>(static_cast<double>(P->GetPropertyValue(ValuePtr))); }
	if (const FDoubleProperty*	P = CastField<FDoubleProperty>(Property))	{ return Make<double>(P->GetPropertyValue(ValuePtr)); }
	if (const FNameProperty*		P = CastField<FNameProperty>(Property))		{ return Make<FName>(P->GetPropertyValue(ValuePtr)); }
	if (const FStrProperty*			P = CastField<FStrProperty>(Property))		{ return Make<FString>(P->GetPropertyValue(ValuePtr)); }
	if (const FTextProperty*		P = CastField<FTextProperty>(Property))		{ return Make<FText>(P->GetPropertyValue(ValuePtr)); }

	if (const FStructProperty* P = CastField<FStructProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::Struct;
		V.StructValue.InitializeAs(P->Struct);
		P->Struct->CopyScriptStruct(V.StructValue.GetMutableMemory(), ValuePtr);
		return V;
	}
	if (const FClassProperty* P = CastField<FClassProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::Class;
		V.ClassValue = Cast<UClass>(P->GetPropertyValue(ValuePtr));
		return V;
	}
	if (const FSoftClassProperty* P = CastField<FSoftClassProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::SoftClass;
		V.SoftClassValue = TSoftClassPtr<UObject>(P->GetPropertyValue(ValuePtr).ToSoftObjectPath());
		return V;
	}
	if (const FObjectProperty* P = CastField<FObjectProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::Object;
		V.ObjectValue = P->GetPropertyValue(ValuePtr);
		return V;
	}
	if (const FSoftObjectProperty* P = CastField<FSoftObjectProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::SoftObject;
		V.SoftObjectValue = TSoftObjectPtr<UObject>(P->GetPropertyValue(ValuePtr).ToSoftObjectPath());
		return V;
	}
	if (const FEnumProperty* P = CastField<FEnumProperty>(Property))
	{
		FKzVariant V;
		V.Type = EPropertyBagPropertyType::Enum;
		V.EnumType = P->GetEnum();
		V.EnumValue = P->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
		return V;
	}

	return FKzVariant();
}

bool FKzVariant::MatchesProperty(const FProperty* Property) const
{
	if (!Property) return false;

	switch (Type)
	{
		case EPropertyBagPropertyType::Bool:		return Property->IsA<FBoolProperty>();
		case EPropertyBagPropertyType::Byte:		return Property->IsA<FByteProperty>();
		case EPropertyBagPropertyType::Int32:		return Property->IsA<FIntProperty>();
		case EPropertyBagPropertyType::Int64:		return Property->IsA<FInt64Property>();
		case EPropertyBagPropertyType::Float:		return Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>();
		case EPropertyBagPropertyType::Double:	return Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>();
		case EPropertyBagPropertyType::Name:		return Property->IsA<FNameProperty>();
		case EPropertyBagPropertyType::String:	return Property->IsA<FStrProperty>();
		case EPropertyBagPropertyType::Text:		return Property->IsA<FTextProperty>();
		case EPropertyBagPropertyType::Enum:
		{
			if (const FEnumProperty* EP = CastField<FEnumProperty>(Property)) { return EP->GetEnum() == EnumType; }
			if (const FByteProperty* BP = CastField<FByteProperty>(Property)) { return BP->Enum == EnumType; }
			return false;
		}
		case EPropertyBagPropertyType::Struct:
		{
			const FStructProperty* SP = CastField<FStructProperty>(Property);
			return SP && SP->Struct == StructValue.GetScriptStruct();
		}
		case EPropertyBagPropertyType::Object:			return Property->IsA<FObjectProperty>();
		case EPropertyBagPropertyType::SoftObject:	return Property->IsA<FSoftObjectProperty>();
		case EPropertyBagPropertyType::Class:				return Property->IsA<FClassProperty>();
		case EPropertyBagPropertyType::SoftClass:		return Property->IsA<FSoftClassProperty>();
		default:																		return false;
	}
}

bool FKzVariant::ToProperty(const FProperty* Property, void* ValuePtr) const
{
	if (!MatchesProperty(Property) || !ValuePtr) return false;

	const void* Src = GetData();
	if (!Src) return false;

	if (Type == EPropertyBagPropertyType::Enum)
	{
		if (const FEnumProperty* P = CastField<FEnumProperty>(Property))
		{
			P->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EnumValue);
			return true;
		}
		if (const FByteProperty* P = CastField<FByteProperty>(Property))
		{
			// FByteProperty backing an enum: width is always 1 byte; truncate explicitly.
			P->SetPropertyValue(ValuePtr, static_cast<uint8>(EnumValue));
			return true;
		}
		return false;
	}

	// Float<->Double conversion: Property may be float while we store double.
	if (Type == EPropertyBagPropertyType::Float || Type == EPropertyBagPropertyType::Double)
	{
		if (const FFloatProperty* P = CastField<FFloatProperty>(Property))
		{
			P->SetPropertyValue(ValuePtr, static_cast<float>(DoubleValue));
			return true;
		}
		if (const FDoubleProperty* P = CastField<FDoubleProperty>(Property))
		{
			P->SetPropertyValue(ValuePtr, DoubleValue);
			return true;
		}
	}

	Property->CopyCompleteValue(ValuePtr, Src);
	return true;
}

bool FKzVariant::operator==(const FKzVariant& Other) const
{
	if (Type != Other.Type) return false;

	switch (Type)
	{
		case EPropertyBagPropertyType::None:				return true;
		case EPropertyBagPropertyType::Bool:				return BoolValue == Other.BoolValue;
		case EPropertyBagPropertyType::Byte:				return ByteValue == Other.ByteValue;
		case EPropertyBagPropertyType::Enum:				return EnumType == Other.EnumType && EnumValue == Other.EnumValue;
		case EPropertyBagPropertyType::Int32:				return Int32Value == Other.Int32Value;
		case EPropertyBagPropertyType::Int64:				return Int64Value == Other.Int64Value;
		case EPropertyBagPropertyType::Float:
		case EPropertyBagPropertyType::Double:			return DoubleValue == Other.DoubleValue;
		case EPropertyBagPropertyType::Name:				return NameValue == Other.NameValue;
		case EPropertyBagPropertyType::String:			return StringValue == Other.StringValue;
		case EPropertyBagPropertyType::Text:				return TextValue.EqualTo(Other.TextValue);
		case EPropertyBagPropertyType::Struct:			return StructValue == Other.StructValue;
		case EPropertyBagPropertyType::Object:			return ObjectValue == Other.ObjectValue;
		case EPropertyBagPropertyType::SoftObject:	return SoftObjectValue == Other.SoftObjectValue;
		case EPropertyBagPropertyType::Class:				return ClassValue == Other.ClassValue;
		case EPropertyBagPropertyType::SoftClass:		return SoftClassValue == Other.SoftClassValue;
		default:																		return false;
	}
}

bool FKzVariant::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 TypeByte = static_cast<uint8>(Type);
	Ar << TypeByte;
	if (Ar.IsLoading())
	{
		Type = static_cast<EPropertyBagPropertyType>(TypeByte);
	}

	switch (Type)
	{
		case EPropertyBagPropertyType::None:		break;
		case EPropertyBagPropertyType::Bool:		Ar << BoolValue;		break;
		case EPropertyBagPropertyType::Byte:		Ar << ByteValue; break;
		case EPropertyBagPropertyType::Enum:
		{
			UEnum* Enum = EnumType.Get();
			Ar << Enum;
			if (Ar.IsLoading()) { EnumType = Enum; }
			Ar << EnumValue;
			break;
		}
		case EPropertyBagPropertyType::Int32:		Ar << Int32Value;		break;
		case EPropertyBagPropertyType::Int64:		Ar << Int64Value;		break;
		case EPropertyBagPropertyType::Float:
		case EPropertyBagPropertyType::Double:	Ar << DoubleValue;	break;
		case EPropertyBagPropertyType::Name:		Ar << NameValue;		break;
		case EPropertyBagPropertyType::String:	Ar << StringValue;	break;
		case EPropertyBagPropertyType::Text:		Ar << TextValue;		break;
		case EPropertyBagPropertyType::Struct:	StructValue.NetSerialize(Ar, Map, bOutSuccess); break;
		case EPropertyBagPropertyType::Object:
		{
			UObject* Obj = ObjectValue.Get();
			Ar << Obj;
			if (Ar.IsLoading()) ObjectValue = Obj;
			break;
		}
		case EPropertyBagPropertyType::SoftObject:	Ar << SoftObjectValue;	break;
		case EPropertyBagPropertyType::Class:
		{
			UClass* Cls = ClassValue.Get();
			Ar << Cls;
			if (Ar.IsLoading()) ClassValue = Cls;
			break;
		}
		case EPropertyBagPropertyType::SoftClass:	Ar << SoftClassValue;	break;
		default:
			ensureMsgf(false, TEXT("FKzVariant::NetSerialize: unhandled type %d"), (int32)Type);
			break;
	}

	bOutSuccess = true;
	return true;
}
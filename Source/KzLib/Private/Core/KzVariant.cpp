// Copyright 2026 kirzo

#include "Core/KzVariant.h"
#include "Misc/PackageName.h"
#include "UObject/TextProperty.h"

const FKzVariant FKzVariant::Invalid = FKzVariant();

namespace KzVariantPrivate
{
	const FName ValueName(TEXT("Value"));

	// A fixed ID makes every variant of a type resolve to the same bag struct.
	const FGuid ValueID(0x4B7A5661, 0x7269616E, 0x7456616C, 0x75650000);

	bool IsObjectType(EPropertyBagPropertyType Type)
	{
		return Type == EPropertyBagPropertyType::Object || Type == EPropertyBagPropertyType::SoftObject || Type == EPropertyBagPropertyType::Class || Type == EPropertyBagPropertyType::SoftClass;
	}

	bool IsSupported(EPropertyBagPropertyType Type)
	{
		switch (Type)
		{
			case EPropertyBagPropertyType::Bool:
			case EPropertyBagPropertyType::Byte:
			case EPropertyBagPropertyType::Int32:
			case EPropertyBagPropertyType::Int64:
			case EPropertyBagPropertyType::Float:
			case EPropertyBagPropertyType::Double:
			case EPropertyBagPropertyType::Name:
			case EPropertyBagPropertyType::String:
			case EPropertyBagPropertyType::Text:
			case EPropertyBagPropertyType::Enum:
			case EPropertyBagPropertyType::Struct:
				return true;
			default:
				return IsObjectType(Type);
		}
	}

	/** Object types without a class take any object. */
	const UObject* ResolveTypeObject(EPropertyBagPropertyType Type, const UObject* TypeObject)
	{
		return !TypeObject && IsObjectType(Type) ? UObject::StaticClass() : TypeObject;
	}

	const UPropertyBag* FindBag(EPropertyBagPropertyType Type, const UObject* TypeObject)
	{
		if (!IsSupported(Type) || (!TypeObject && (Type == EPropertyBagPropertyType::Enum || Type == EPropertyBagPropertyType::Struct)))
		{
			return nullptr;
		}

		FPropertyBagPropertyDesc Desc(ValueName, Type, ResolveTypeObject(Type, TypeObject));
		Desc.ID = ValueID;
		return UPropertyBag::GetOrCreateFromDescs(MakeArrayView(&Desc, 1));
	}

	constexpr int32 InlineSize = sizeof(FSoftObjectPtr);
	constexpr int32 InlineAlignment = alignof(FSoftObjectPtr);

	bool IsInline(const UPropertyBag* Bag)
	{
		return Bag->GetStructureSize() <= InlineSize && Bag->GetMinAlignment() <= InlineAlignment;
	}

	void SkipWhitespace(const TCHAR*& Buffer)
	{
		while (FChar::IsWhitespace(*Buffer))
		{
			++Buffer;
		}
	}
}

FKzVariant::FKzVariant(const FKzVariant& Other)
{
	CopyFrom(Other);
}

FKzVariant::FKzVariant(FKzVariant&& Other)
{
	MoveFrom(Other);
}

FKzVariant& FKzVariant::operator=(const FKzVariant& Other)
{
	if (this != &Other)
	{
		if (Bag && Bag == Other.Bag)
		{
			Bag->CopyScriptStruct(GetBagMemory(), Other.GetBagMemory());
		}
		else
		{
			Reset();
			CopyFrom(Other);
		}
	}
	return *this;
}

FKzVariant& FKzVariant::operator=(FKzVariant&& Other)
{
	if (this != &Other)
	{
		Reset();
		MoveFrom(Other);
	}
	return *this;
}

FKzVariant::~FKzVariant()
{
	Reset();
}

void FKzVariant::Reset()
{
	if (!Bag)
	{
		return;
	}

	// Through UScriptStruct, where it is public; the bag's override still runs and releases its refcount.
	uint8* Memory = GetBagMemory();
	static_cast<const UScriptStruct*>(Bag.Get())->DestroyStruct(Memory);
	if (!KzVariantPrivate::IsInline(Bag))
	{
		FMemory::Free(Memory);
	}
	Bag = nullptr;
}

EPropertyBagPropertyType FKzVariant::GetType() const
{
	return Bag ? Bag->GetPropertyDescs()[0].ValueType : EPropertyBagPropertyType::None;
}

const UObject* FKzVariant::GetTypeObject() const
{
	return Bag ? Bag->GetPropertyDescs()[0].ValueTypeObject.Get() : nullptr;
}

void FKzVariant::SetType(EPropertyBagPropertyType NewType, const UObject* NewTypeObject)
{
	Reset();
	void* Value = InitializeAs(NewType, NewTypeObject);

	// Enums start at their first entry rather than at a value they may not have.
	const UEnum* Enum = Cast<UEnum>(NewTypeObject);
	if (Value && Enum && NewType == EPropertyBagPropertyType::Enum && Enum->NumEnums() > 1)
	{
		*static_cast<uint8*>(Value) = static_cast<uint8>(Enum->GetValueByIndex(0));
	}
}

void* FKzVariant::GetData()
{
	const FProperty* Property = GetValueProperty();
	return Property ? Property->ContainerPtrToValuePtr<void>(GetBagMemory()) : nullptr;
}

const void* FKzVariant::GetData() const
{
	return const_cast<FKzVariant*>(this)->GetData();
}

const FProperty* FKzVariant::GetValueProperty() const
{
	return Bag ? Bag->GetPropertyDescs()[0].CachedProperty : nullptr;
}

uint8* FKzVariant::GetBagMemory() const
{
	if (!Bag)
	{
		return nullptr;
	}
	return KzVariantPrivate::IsInline(Bag) ? const_cast<uint8*>(Storage) : *reinterpret_cast<uint8* const*>(Storage);
}

void* FKzVariant::InitializeAs(EPropertyBagPropertyType NewType, const UObject* NewTypeObject)
{
	if (Bag && GetType() == NewType && GetTypeObject() == KzVariantPrivate::ResolveTypeObject(NewType, NewTypeObject))
	{
		return GetData();
	}

	Reset();
	const UPropertyBag* NewBag = KzVariantPrivate::FindBag(NewType, NewTypeObject);
	if (!NewBag)
	{
		return nullptr;
	}
	Allocate(NewBag);
	return GetData();
}

void FKzVariant::Allocate(const UPropertyBag* NewBag)
{
	uint8* Memory = Storage;
	if (!KzVariantPrivate::IsInline(NewBag))
	{
		Memory = static_cast<uint8*>(FMemory::Malloc(NewBag->GetStructureSize(), NewBag->GetMinAlignment()));
		*reinterpret_cast<uint8**>(Storage) = Memory;
	}
	static_cast<const UScriptStruct*>(NewBag)->InitializeStruct(Memory);
	Bag = NewBag;
}

void FKzVariant::CopyFrom(const FKzVariant& Other)
{
	if (Other.Bag)
	{
		Allocate(Other.Bag);
		Bag->CopyScriptStruct(GetBagMemory(), Other.GetBagMemory());
	}
}

void FKzVariant::MoveFrom(FKzVariant& Other)
{
	// Unreal types are relocatable, so the value moves with its bytes.
	FMemory::Memcpy(Storage, Other.Storage, sizeof(Storage));
	Bag = Other.Bag;
	Other.Bag = nullptr;
}

const UObject* FKzVariant::GetObject() const
{
	return static_cast<const TObjectPtr<UObject>*>(GetData())->Get();
}

bool FKzVariant::MatchesTypeObject(const UObject* Expected) const
{
	const UObject* Held = GetTypeObject();
	if (!Expected || Held == Expected)
	{
		return true;
	}

	// Structs and enums match exactly; classes by inheritance.
	const UClass* ExpectedClass = Cast<UClass>(Expected);
	const UClass* HeldClass = Cast<UClass>(Held);
	if (!ExpectedClass || !HeldClass)
	{
		return false;
	}
	if (HeldClass->IsChildOf(ExpectedClass))
	{
		return true;
	}

	// A hard reference may hold a narrower object than its declared class.
	if (GetType() == EPropertyBagPropertyType::Object)
	{
		const UObject* Object = GetObject();
		return !Object || Object->IsA(ExpectedClass);
	}
	return false;
}

bool FKzVariant::MatchesType(const FKzTypeDef& TypeDef) const
{
	return TypeDef.ContainerType == EPropertyBagContainerType::None && GetType() == TypeDef.ValueType && MatchesTypeObject(TypeDef.ValueTypeObject);
}

bool FKzVariant::MatchesProperty(const FProperty* Property) const
{
	if (!Property || !Bag)
	{
		return false;
	}

	const UObject* TypeObject = GetTypeObject();
	switch (GetType())
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
			if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
			{
				return EnumProperty->GetEnum() == TypeObject;
			}
			const FByteProperty* ByteProperty = CastField<FByteProperty>(Property);
			return ByteProperty && ByteProperty->Enum == TypeObject;
		}
		case EPropertyBagPropertyType::Struct:
		{
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			return StructProperty && StructProperty->Struct == TypeObject;
		}
		case EPropertyBagPropertyType::Object:
		{
			const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
			const UObject* Object = GetObject();
			return ObjectProperty && (!Object || Object->IsA(ObjectProperty->PropertyClass));
		}
		case EPropertyBagPropertyType::Class:
		{
			const FClassProperty* ClassProperty = CastField<FClassProperty>(Property);
			const UClass* Class = Cast<UClass>(GetObject());
			return ClassProperty && (!Class || Class->IsChildOf(ClassProperty->MetaClass));
		}
		// A soft reference can only be checked by its declared class without loading it.
		case EPropertyBagPropertyType::SoftObject:
		{
			const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property);
			return SoftProperty && CastChecked<UClass>(TypeObject)->IsChildOf(SoftProperty->PropertyClass);
		}
		case EPropertyBagPropertyType::SoftClass:
		{
			const FSoftClassProperty* SoftProperty = CastField<FSoftClassProperty>(Property);
			return SoftProperty && CastChecked<UClass>(TypeObject)->IsChildOf(SoftProperty->MetaClass);
		}
		default:
			return false;
	}
}

FKzVariant FKzVariant::FromProperty(const FProperty* Property, const void* ValuePtr)
{
	FKzVariant Variant;
	if (!Property || !ValuePtr)
	{
		return Variant;
	}

	const FPropertyBagPropertyDesc Desc(NAME_None, Property);
	void* Value = Desc.ContainerTypes.IsEmpty() ? Variant.InitializeAs(Desc.ValueType, Desc.ValueTypeObject) : nullptr;
	if (!Value)
	{
		return Variant;
	}

	// Bools may be bitfields and enums any width; the rest share the bag property's layout.
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		*static_cast<bool*>(Value) = BoolProperty->GetPropertyValue(ValuePtr);
	}
	else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		*static_cast<uint8*>(Value) = static_cast<uint8>(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
	}
	else
	{
		Property->CopyCompleteValue(Value, ValuePtr);
	}
	return Variant;
}

bool FKzVariant::ToProperty(const FProperty* Property, void* ValuePtr) const
{
	if (!ValuePtr || !MatchesProperty(Property))
	{
		return false;
	}

	const void* Value = GetData();
	switch (GetType())
	{
		case EPropertyBagPropertyType::Bool:
			CastFieldChecked<FBoolProperty>(Property)->SetPropertyValue(ValuePtr, *static_cast<const bool*>(Value));
			return true;
		case EPropertyBagPropertyType::Enum:
			if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
			{
				EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, static_cast<int64>(*static_cast<const uint8*>(Value)));
			}
			else
			{
				CastFieldChecked<FByteProperty>(Property)->SetPropertyValue(ValuePtr, *static_cast<const uint8*>(Value));
			}
			return true;
		case EPropertyBagPropertyType::Float:
		case EPropertyBagPropertyType::Double:
		{
			const double Number = GetType() == EPropertyBagPropertyType::Float ? *static_cast<const float*>(Value) : *static_cast<const double*>(Value);
			if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
			{
				FloatProperty->SetPropertyValue(ValuePtr, static_cast<float>(Number));
			}
			else
			{
				CastFieldChecked<FDoubleProperty>(Property)->SetPropertyValue(ValuePtr, Number);
			}
			return true;
		}
		default:
			Property->CopyCompleteValue(ValuePtr, Value);
			return true;
	}
}

bool FKzVariant::Serialize(FArchive& Ar)
{
	// Type, type object, then the value's size so a value whose type no longer loads can be skipped.
	if (Ar.IsLoading())
	{
		uint8 Type = 0;
		Ar << Type;
		UObject* TypeObject = nullptr;
		Ar << TypeObject;
		if (TypeObject)
		{
			Ar.Preload(TypeObject);
		}
		int32 SerialSize = 0;
		Ar << SerialSize;

		if (InitializeAs(static_cast<EPropertyBagPropertyType>(Type), TypeObject))
		{
			const_cast<UPropertyBag*>(Bag.Get())->SerializeItem(Ar, GetBagMemory(), nullptr);
		}
		else
		{
			Reset();
			Ar.Seek(Ar.Tell() + SerialSize);
		}
	}
	else if (Ar.IsSaving())
	{
		uint8 Type = static_cast<uint8>(GetType());
		Ar << Type;
		UObject* TypeObject = const_cast<UObject*>(GetTypeObject());
		Ar << TypeObject;

		const int64 SizeOffset = Ar.Tell();
		int32 SerialSize = 0;
		Ar << SerialSize;
		const int64 ValueOffset = Ar.Tell();
		if (Bag)
		{
			const_cast<UPropertyBag*>(Bag.Get())->SerializeItem(Ar, GetBagMemory(), nullptr);
		}
		const int64 EndOffset = Ar.Tell();
		Ar.Seek(SizeOffset);
		SerialSize = static_cast<int32>(EndOffset - ValueOffset);
		Ar << SerialSize;
		Ar.Seek(EndOffset);
	}
	else if (Ar.IsCountingMemory() || Ar.IsModifyingWeakAndStrongReferences() || Ar.IsObjectReferenceCollector())
	{
		UObject* TypeObject = const_cast<UObject*>(GetTypeObject());
		Ar << TypeObject;
		if (Bag)
		{
			const_cast<UPropertyBag*>(Bag.Get())->SerializeItem(Ar, GetBagMemory(), nullptr);
		}
	}
	return true;
}

bool FKzVariant::Identical(const FKzVariant* Other, uint32 PortFlags) const
{
	if (!Other)
	{
		return !Bag;
	}
	return Bag == Other->Bag && (!Bag || Bag->CompareScriptStruct(GetBagMemory(), Other->GetBagMemory(), PortFlags));
}

void FKzVariant::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	if (Bag)
	{
		Collector.AddReferencedObject(Bag);
		Collector.AddPropertyReferencesWithStructARO(Bag, GetBagMemory());
	}
}

bool FKzVariant::ExportTextItem(FString& ValueStr, const FKzVariant& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	// Type name, the type object's path if any, then the bag's own text: "Struct /Script/CoreUObject.Vector(Value=(X=0,Y=0,Z=0))".
	if (!Bag)
	{
		ValueStr += TEXT("None");
		return true;
	}

	ValueStr += StaticEnum<EPropertyBagPropertyType>()->GetNameStringByValue(static_cast<int64>(GetType()));
	if (const UObject* TypeObject = GetTypeObject())
	{
		ValueStr += TEXT(" ");
		ValueStr += TypeObject->GetPathName();
	}
	Bag->ExportText(ValueStr, GetBagMemory(), GetBagMemory(), Parent, PortFlags, ExportRootScope);
	return true;
}

bool FKzVariant::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive)
{
	FNameBuilder TypeName;
	const TCHAR* Result = FPropertyHelpers::ReadToken(Buffer, TypeName, true);
	if (!Result)
	{
		return false;
	}
	Buffer = Result;

	if (FCString::Stricmp(TypeName.ToString(), TEXT("None")) == 0)
	{
		Reset();
		return true;
	}

	const int64 Type = StaticEnum<EPropertyBagPropertyType>()->GetValueByNameString(TypeName.ToString());
	if (Type == INDEX_NONE)
	{
		return false;
	}

	KzVariantPrivate::SkipWhitespace(Buffer);
	UObject* TypeObject = nullptr;
	if (*Buffer != TCHAR('('))
	{
		FNameBuilder TypeObjectPath;
		Result = FPropertyHelpers::ReadToken(Buffer, TypeObjectPath, true);
		if (!Result || ((PortFlags & PPF_Copy) && !FPackageName::IsValidObjectPath(TypeObjectPath.ToView())))
		{
			return false;
		}
		Buffer = Result;
		TypeObject = LoadObject<UObject>(nullptr, TypeObjectPath.ToString());
		if (!TypeObject)
		{
			return false;
		}
	}

	if (!InitializeAs(static_cast<EPropertyBagPropertyType>(Type), TypeObject))
	{
		return false;
	}

	const UPropertyBag* BagStruct = Bag;
	Result = BagStruct->ImportText(Buffer, GetBagMemory(), Parent, PortFlags, ErrorText, [BagStruct]() { return BagStruct->GetName(); });
	if (!Result)
	{
		return false;
	}
	Buffer = Result;
	return true;
}

void FKzVariant::GetPreloadDependencies(TArray<UObject*>& OutDeps)
{
	if (const UObject* TypeObject = GetTypeObject())
	{
		OutDeps.Add(const_cast<UObject*>(TypeObject));
	}
}

bool FKzVariant::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 Type = static_cast<uint8>(GetType());
	Ar << Type;

	UObject* TypeObject = const_cast<UObject*>(GetTypeObject());
	if (Type != 0)
	{
		Ar << TypeObject;
	}

	if (Ar.IsLoading() && !InitializeAs(static_cast<EPropertyBagPropertyType>(Type), TypeObject))
	{
		Reset();
		bOutSuccess = Type == 0;
		return true;
	}

	const FProperty* Property = GetValueProperty();
	bOutSuccess = !Property || Property->NetSerializeItem(Ar, Map, GetData());
	return true;
}
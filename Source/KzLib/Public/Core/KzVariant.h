// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzTypeDef.h"
#include "Core/KzPropertyBagHelpers.h"
#include "StructUtils/PropertyBag.h"
#include "UObject/SoftObjectPtr.h"
#include "KzVariant.generated.h"

/**
 * A value of any type a property bag supports, containers aside. It lives inline as an instance of a property bag struct with a single
 * property of its type, shared by every variant of that type, which copies, compares, serializes and reports it to the GC. Structs that do
 * not fit inline go to the heap.
 *
 *   FKzVariant V = FKzVariant::Make<float>(100.0f);
 *   if (V.Is<float>()) { float X = V.Get<float>(); }
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzVariant
{
	GENERATED_BODY()

public:
	static const FKzVariant Invalid;

	FKzVariant() = default;
	FKzVariant(const FKzVariant& Other);
	FKzVariant(FKzVariant&& Other);
	FKzVariant& operator=(const FKzVariant& Other);
	FKzVariant& operator=(FKzVariant&& Other);
	~FKzVariant();

	bool IsValid() const { return Bag != nullptr; }
	void Reset();

	EPropertyBagPropertyType GetType() const;

	/** The enum or struct of the type, or for object and class types the class the value must be a child of. */
	const UObject* GetTypeObject() const;

	/** Gives the variant a type, holding that type's default value. */
	void SetType(EPropertyBagPropertyType NewType, const UObject* NewTypeObject = nullptr);

	template <typename T>
	bool Is() const;

	template <typename T>
	T Get() const;

	template <typename T>
	void Set(const T& Value);

	template <typename T>
	static FKzVariant Make(const T& Value);

	static FKzVariant FromProperty(const FProperty* Property, const void* ValuePtr);
	bool ToProperty(const FProperty* Property, void* ValuePtr) const;

	/** True if the value can be written into the property. */
	bool MatchesProperty(const FProperty* Property) const;
	bool MatchesType(const FKzTypeDef& TypeDef) const;

	/** The value, laid out as GetValueProperty() expects; null when invalid. */
	void* GetData();
	const void* GetData() const;
	const FProperty* GetValueProperty() const;

	/** The shared struct the value is an instance of, and that instance: for editors showing the value through the struct's property. */
	const UPropertyBag* GetBagStruct() const { return Bag; }
	uint8* GetBagMemory() const;

	bool Serialize(FArchive& Ar);
	bool Identical(const FKzVariant* Other, uint32 PortFlags) const;
	void AddStructReferencedObjects(FReferenceCollector& Collector);
	bool ExportTextItem(FString& ValueStr, const FKzVariant& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive = nullptr);
	void GetPreloadDependencies(TArray<UObject*>& OutDeps);
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FKzVariant& Other) const { return Identical(&Other, 0); }
	bool operator!=(const FKzVariant& Other) const { return !(*this == Other); }

private:
	/** Makes the variant hold the type and returns its value: kept if it already held that type, default otherwise. Null for unsupported types. */
	void* InitializeAs(EPropertyBagPropertyType NewType, const UObject* NewTypeObject);

	/** Default-initializes a value of the bag's type in an invalid variant. */
	void Allocate(const UPropertyBag* NewBag);

	bool MatchesTypeObject(const UObject* Expected) const;

	/** The object a hard object or class reference holds. */
	const UObject* GetObject() const;

	void CopyFrom(const FKzVariant& Other);
	void MoveFrom(FKzVariant& Other);

	alignas(FSoftObjectPtr) uint8 Storage[sizeof(FSoftObjectPtr)];

	TObjectPtr<const UPropertyBag> Bag;
};

template<>
struct TStructOpsTypeTraits<FKzVariant> : public TStructOpsTypeTraitsBase2<FKzVariant>
{
	enum
	{
		WithSerializer = true,
		WithIdentical = true,
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithAddStructReferencedObjects = true,
		WithGetPreloadDependencies = true,
		WithNetSerializer = true,
	};
};

template <typename T>
bool FKzVariant::Is() const
{
	using Traits = KzPropertyBag::TPropertyBagType<T>;
	return GetType() == Traits::Type && MatchesTypeObject(Traits::GetObjectType());
}

template <typename T>
T FKzVariant::Get() const
{
	checkf(Is<T>(), TEXT("FKzVariant::Get<T>: type mismatch"));

	using Traits = KzPropertyBag::TPropertyBagType<T>;
	const void* Value = GetData();

	if constexpr (Traits::Type == EPropertyBagPropertyType::Enum)
	{
		return static_cast<T>(*static_cast<const uint8*>(Value));
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Object)
	{
		using ObjectType = std::remove_pointer_t<decltype(ToRawPtr(std::declval<T>()))>;
		return Cast<ObjectType>(static_cast<const TObjectPtr<UObject>*>(Value)->Get());
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftObject || Traits::Type == EPropertyBagPropertyType::SoftClass)
	{
		return T(static_cast<const FSoftObjectPtr*>(Value)->ToSoftObjectPath());
	}
	else
	{
		return *static_cast<const T*>(Value);
	}
}

template <typename T>
void FKzVariant::Set(const T& Value)
{
	using Traits = KzPropertyBag::TPropertyBagType<T>;
	void* Data = InitializeAs(Traits::Type, Traits::GetObjectType());
	check(Data);

	if constexpr (Traits::Type == EPropertyBagPropertyType::Enum)
	{
		*static_cast<uint8*>(Data) = static_cast<uint8>(Value);
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Object)
	{
		*static_cast<TObjectPtr<UObject>*>(Data) = ToRawPtr(Value);
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftObject || Traits::Type == EPropertyBagPropertyType::SoftClass)
	{
		*static_cast<FSoftObjectPtr*>(Data) = FSoftObjectPtr(Value.ToSoftObjectPath());
	}
	else
	{
		*static_cast<T*>(Data) = Value;
	}
}

template <typename T>
FKzVariant FKzVariant::Make(const T& Value)
{
	FKzVariant Variant;
	Variant.Set<T>(Value);
	return Variant;
}
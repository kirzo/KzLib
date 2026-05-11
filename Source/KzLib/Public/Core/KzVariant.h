// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzTypeDef.h"
#include "Core/KzPropertyBagHelpers.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/PropertyBag.h"
#include "KzVariant.generated.h"

/**
 * A type-erased value that can hold any property type supported by the KzLib param system.
 * Uses multi-slot storage so serialization, GC, replication and undo/redo work natively.
 *
 * Usage:
 *   FKzVariant V = FKzVariant::Make<float>(100.0f);
 *   if (V.Is<float>()) { float X = V.Get<float>(); }
 *   V.Set<int32>(42);
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzVariant
{
	GENERATED_BODY()

public:
	static const FKzVariant Invalid;

	FKzVariant() = default;

	/** True if the variant holds a value of any supported type. */
	bool IsValid() const { return Type != EPropertyBagPropertyType::None; }

	/** Resets the variant to an empty (invalid) state and clears all slots. */
	void Reset();

	/** Returns the bag-level type of the held value. */
	EPropertyBagPropertyType GetType() const { return Type; }

	/** Resets all slots and sets the type. The new active slot stays at its default value. */
	void SetType(EPropertyBagPropertyType NewType, const UObject* NewTypeObject = nullptr);

	/** Returns the optional UObject describing the type (UEnum, UScriptStruct, UClass) when applicable. */
	const UObject* GetTypeObject() const;

	/** Returns true if the variant holds a value compatible with type T. */
	template <typename T>
	bool Is() const;

	/** Reads the value as type T. Asserts in debug if the type doesn't match. */
	template <typename T>
	T Get() const;

	/** Stores a value of type T, overwriting any previous content. */
	template <typename T>
	void Set(const T& Value);

	/** Constructs a new variant holding the given value. */
	template <typename T>
	static FKzVariant Make(const T& Value);

	/** Reads a value from a raw FProperty + memory pair into a fresh variant. */
	static FKzVariant FromProperty(const FProperty* Property, const void* ValuePtr);

	/** Writes the held value into a raw FProperty + memory pair, if the types match. Returns true on success. */
	bool ToProperty(const FProperty* Property, void* ValuePtr) const;

	/** True if Property's type and the variant's type are compatible (same category, same struct/class/enum if applicable). */
	bool MatchesProperty(const FProperty* Property) const;

	/** True if TypeDef and the variant's type are compatible. */
	bool MatchesType(const FKzTypeDef& TypeDef) const;

	/** Returns a pointer to the active slot's raw memory, or nullptr if invalid. */
	void* GetData();
	const void* GetData() const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	bool operator==(const FKzVariant& Other) const;
	bool operator!=(const FKzVariant& Other) const { return !(*this == Other); }

private:
	UPROPERTY(EditAnywhere, Category = "KzVariant")
	EPropertyBagPropertyType Type = EPropertyBagPropertyType::None;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	uint8 ByteValue = 0;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	TObjectPtr<UEnum> EnumType = nullptr;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	int64 EnumValue = 0;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	int32 Int32Value = 0;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	int64 Int64Value = 0;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	double DoubleValue = 0.0;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	FName NameValue;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	FString StringValue;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	FText TextValue;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	FInstancedStruct StructValue;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	TObjectPtr<UObject> ObjectValue = nullptr;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	TSoftObjectPtr<UObject> SoftObjectValue;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	TObjectPtr<UClass> ClassValue = nullptr;

	UPROPERTY(EditAnywhere, Category = "KzVariant")
	TSoftClassPtr<UObject> SoftClassValue;

	/** Internal helper: validates that T matches the currently held type. */
	template <typename T>
	bool MatchesType() const;
};

template<>
struct TStructOpsTypeTraits<FKzVariant> : public TStructOpsTypeTraitsBase2<FKzVariant>
{
	enum
	{
		WithNetSerializer = true,
	};
};

// =====================================================================================
// Templated API implementation
// =====================================================================================

template <typename T>
bool FKzVariant::MatchesType() const
{
	using Traits = KzPropertyBag::TPropertyBagType<T>;
	if (Type != Traits::Type)
	{
		return false;
	}

	const UObject* ExpectedTypeObj = Traits::GetObjectType();
	const UObject* HeldTypeObj = GetTypeObject();

	// Primitive without UObject metadata: matching by ValueType is enough.
	if (ExpectedTypeObj == nullptr && HeldTypeObj == nullptr)
	{
		return true;
	}

	// No restriction stored — accept any compatible T.
	if (HeldTypeObj == nullptr)
	{
		return true;
	}

	// T is generic (e.g. UObject*) but the variant stores a specific type — always compatible.
	if (ExpectedTypeObj == nullptr)
	{
		return true;
	}

	// Object/Class refs: allow polymorphism (held type is child of expected).
	if (Type == EPropertyBagPropertyType::Object ||
		Type == EPropertyBagPropertyType::SoftObject ||
		Type == EPropertyBagPropertyType::Class ||
		Type == EPropertyBagPropertyType::SoftClass)
	{
		const UClass* ExpectedClass = Cast<UClass>(ExpectedTypeObj);
		const UClass* HeldClass = Cast<UClass>(HeldTypeObj);
		return ExpectedClass && HeldClass && HeldClass->IsChildOf(ExpectedClass);
	}

	// Structs and enums require exact match.
	return ExpectedTypeObj == HeldTypeObj;
}

template <typename T>
bool FKzVariant::Is() const
{
	return MatchesType<T>();
}

template <typename T>
T FKzVariant::Get() const
{
	checkf(MatchesType<T>(), TEXT("FKzVariant::Get<T>: type mismatch"));

	using Traits = KzPropertyBag::TPropertyBagType<T>;

	if constexpr (std::is_same_v<T, bool>) { return BoolValue; }
	else if constexpr (std::is_same_v<T, uint8>) { return ByteValue; }
	else if constexpr (std::is_same_v<T, int32>) { return Int32Value; }
	else if constexpr (std::is_same_v<T, int64>) { return Int64Value; }
	else if constexpr (std::is_same_v<T, float>) { return static_cast<float>(DoubleValue); }
	else if constexpr (std::is_same_v<T, double>) { return DoubleValue; }
	else if constexpr (std::is_same_v<T, FName>) { return NameValue; }
	else if constexpr (std::is_same_v<T, FString>) { return StringValue; }
	else if constexpr (std::is_same_v<T, FText>) { return TextValue; }
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Enum)
	{
		return static_cast<T>(EnumValue);
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Struct)
	{
		return StructValue.Get<T>();
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Object)
	{
		// T is T* or TObjectPtr<T>. Both convert from raw UObject*.
		return Cast<std::remove_pointer_t<std::decay_t<T>>>(ObjectValue.Get());
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftObject)
	{
		return T(SoftObjectValue.ToSoftObjectPath());
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Class)
	{
		return T(ClassValue.Get());
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftClass)
	{
		return T(SoftClassValue.ToSoftObjectPath());
	}
	else
	{
		static_assert(sizeof(T) == 0, "FKzVariant::Get<T>: unsupported type");
		return T{};
	}
}

template <typename T>
void FKzVariant::Set(const T& Value)
{
	using Traits = KzPropertyBag::TPropertyBagType<T>;

	Reset();
	Type = Traits::Type;

	if constexpr (std::is_same_v<T, bool>) { BoolValue = Value; }
	else if constexpr (std::is_same_v<T, uint8>) { ByteValue = Value; }
	else if constexpr (std::is_same_v<T, int32>) { Int32Value = Value; }
	else if constexpr (std::is_same_v<T, int64>) { Int64Value = Value; }
	else if constexpr (std::is_same_v<T, float>) { DoubleValue = Value; }
	else if constexpr (std::is_same_v<T, double>) { DoubleValue = Value; }
	else if constexpr (std::is_same_v<T, FName>) { NameValue = Value; }
	else if constexpr (std::is_same_v<T, FString>) { StringValue = Value; }
	else if constexpr (std::is_same_v<T, FText>) { TextValue = Value; }
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Enum)
	{
		EnumValue = static_cast<int64>(Value);
		EnumType = const_cast<UEnum*>(Cast<UEnum>(Traits::GetObjectType()));
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Struct)
	{
		StructValue.InitializeAs<T>(Value);
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Object)
	{
		ObjectValue = Value;
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftObject)
	{
		SoftObjectValue = TSoftObjectPtr<UObject>(Value.ToSoftObjectPath());
		ObjectValue = const_cast<UClass*>(Cast<UClass>(Traits::GetObjectType()));
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::Class)
	{
		ClassValue = Value.Get();
	}
	else if constexpr (Traits::Type == EPropertyBagPropertyType::SoftClass)
	{
		SoftClassValue = TSoftClassPtr<UObject>(Value.ToSoftObjectPath());
		ClassValue = const_cast<UClass*>(Cast<UClass>(Traits::GetObjectType()));
	}
	else
	{
		static_assert(sizeof(T) == 0, "FKzVariant::Set<T>: unsupported type");
	}
}

template <typename T>
FKzVariant FKzVariant::Make(const T& Value)
{
	FKzVariant V;
	V.Set<T>(Value);
	return V;
}
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzPropertyBagHelpers.h"
#include "KzTypeDef.generated.h"

/**
 * A type signature for any property type.
 * For "name + type" pairs, use FKzParamDef instead.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTypeDef
{
	GENERATED_BODY()

	static const FKzTypeDef Invalid;

	FKzTypeDef() = default;

	FKzTypeDef(const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: ValueTypeObject(InValueTypeObject)
		, ValueType(InValueType)
	{}

	FKzTypeDef(const EPropertyBagContainerType InContainerType, const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: ValueTypeObject(InValueTypeObject)
		, ValueType(InValueType)
		, ContainerType(InContainerType)
	{}

	bool IsValid() const
	{
		return ValueType != EPropertyBagPropertyType::None;
	}

	friend bool operator==(const FKzTypeDef& Lhs, const FKzTypeDef& Rhs)
	{
		return Lhs.ValueType == Rhs.ValueType
			&& Lhs.ValueTypeObject == Rhs.ValueTypeObject
			&& Lhs.ContainerType == Rhs.ContainerType;
	}

	friend bool operator!=(const FKzTypeDef& Lhs, const FKzTypeDef& Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend uint32 GetTypeHash(const FKzTypeDef& Def)
	{
		uint32 Hash = GetTypeHash(Def.ValueType);

		Hash = HashCombine(Hash, GetTypeHash(Def.ValueTypeObject));
		Hash = HashCombine(Hash, GetTypeHash(Def.ContainerType));

		return Hash;
	}

	/** True if a property declared as this type would accept the given FProperty. */
	bool MatchesProperty(const FProperty* Property) const;

	/** Initializes the type information based on T. */
	template <typename T>
	void Init()
	{
		// Detect Array Wrapper (TArray<T> -> T)
		using UnwrappedT = typename KzPropertyBag::Private::TUnwrapArray<T>::Type;
		constexpr bool bIsArray = KzPropertyBag::Private::TUnwrapArray<T>::IsArray;

		// Get Traits from the inner type (std::decay_t to clean up const/references)
		using Traits = KzPropertyBag::TPropertyBagType<std::decay_t<UnwrappedT>>;

		// Fill Definition
		ValueType = Traits::Type;
		ValueTypeObject = Traits::GetObjectType();
		ContainerType = bIsArray ? EPropertyBagContainerType::Array : EPropertyBagContainerType::None;
	}

	/** Static helper to create a Type in one line. */
	template <typename T>
	static FKzTypeDef Make()
	{
		FKzTypeDef Def;
		Def.Init<T>();
		return Def;
	}

	/** Pointer to the object that defines the Enum, Struct, or Class (if applicable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	TObjectPtr<const UObject> ValueTypeObject = nullptr;

	/** The underlying type of the value (Bool, Int, Object, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::Bool;

	/** Type of the container described by this property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	EPropertyBagContainerType ContainerType = EPropertyBagContainerType::None;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzPropertyBagHelpers.h"
#include "KzParamDef.generated.h"

/**
 * Defines a single parameter definition (Name + Type).
 * It does not hold the value itself, only the signature.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzParamDef
{
	GENERATED_BODY()

	FKzParamDef() = default;

	FKzParamDef(const FName InName, const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: Name(InName)
		, ValueTypeObject(InValueTypeObject)
		, ValueType(InValueType)
	{
	}

	FKzParamDef(const FName InName, const EPropertyBagContainerType InContainerType, const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: Name(InName)
		, ValueTypeObject(InValueTypeObject)
		, ValueType(InValueType)
		, ContainerType(InContainerType)
	{
	}

	bool IsValid() const
	{
		return ValueType != EPropertyBagPropertyType::None;
	}

	friend bool operator==(const FKzParamDef& Lhs, const FKzParamDef& Rhs)
	{
		return Lhs.Name == Rhs.Name
			&& Lhs.ValueType == Rhs.ValueType
			&& Lhs.ValueTypeObject == Rhs.ValueTypeObject
			&& Lhs.ContainerType == Rhs.ContainerType;
	}

	friend bool operator!=(const FKzParamDef& Lhs, const FKzParamDef& Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend uint32 GetTypeHash(const FKzParamDef& Def)
	{
		uint32 Hash = GetTypeHash(Def.Name);

		Hash = HashCombine(Hash, GetTypeHash(Def.ValueType));
		Hash = HashCombine(Hash, GetTypeHash(Def.ValueTypeObject));
		Hash = HashCombine(Hash, GetTypeHash(Def.ContainerType));

		return Hash;
	}

	/** Initializes ONLY the type information based on T. Does NOT change the Name.
	 * Usage:
	 * MyDef.Init<float>();
	 * MyDef.Init<TArray<AActor*>>();
	 */
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

	/** Initializes the type information based on T.
	 * Usage:
	 * MyDef.Init<float>("Health");
	 * MyDef.Init<TArray<AActor*>>("Targets");
	 */
	template <typename T>
	void Init(const FName InName)
	{
		Name = InName;
		Init<T>();
	}

	/** Static helper to create a Definition in one line. */
	template <typename T>
	static FKzParamDef Make()
	{
		FKzParamDef Def;
		Def.Init<T>();
		return Def;
	}

	/** Static helper to create a Definition in one line. */
	template <typename T>
	static FKzParamDef Make(const FName InName)
	{
		FKzParamDef Def;
		Def.Init<T>(InName);
		return Def;
	}

	/**
	 * Converts this definition into a native PropertyBag descriptor.
	 * Useful to call PropertyBag.AddProperties({ MyDef.ToPropertyDesc() });
	 */
	FPropertyBagPropertyDesc ToPropertyDesc() const
	{
		return FPropertyBagPropertyDesc(Name, ContainerType, ValueType, ValueTypeObject);
	}

	/** Implicit conversion operator. */
	operator FPropertyBagPropertyDesc() const
	{
		return ToPropertyDesc();
	}

	/** Name of the parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	FName Name = NAME_None;

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
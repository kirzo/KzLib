// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzTypeDef.h"
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

	FKzParamDef(const FName InName, const FKzTypeDef& InType)
		: Name(InName)
		, Type(InType)
	{}

	FKzParamDef(const FName InName, const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: Name(InName)
		, Type(InValueType, InValueTypeObject)
	{}

	FKzParamDef(const FName InName, const EPropertyBagContainerType InContainerType, const EPropertyBagPropertyType InValueType, const UObject* InValueTypeObject = nullptr)
		: Name(InName)
		, Type(InContainerType, InValueType, InValueTypeObject)
	{}

	bool IsValid() const
	{
		return Type.IsValid();
	}

	friend bool operator==(const FKzParamDef& Lhs, const FKzParamDef& Rhs)
	{
		return Lhs.Name == Rhs.Name && Lhs.Type == Rhs.Type;
	}

	friend bool operator!=(const FKzParamDef& Lhs, const FKzParamDef& Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend uint32 GetTypeHash(const FKzParamDef& Def)
	{
		uint32 Hash = GetTypeHash(Def.Name);
		Hash = HashCombine(Hash, GetTypeHash(Def.Type));
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
		Type.Init<T>();
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
		return FPropertyBagPropertyDesc(Name, Type.ContainerType, Type.ValueType, Type.ValueTypeObject);
	}

	/** Implicit conversion operator. */
	operator FPropertyBagPropertyDesc() const
	{
		return ToPropertyDesc();
	}

	/** Name of the parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	FName Name = NAME_None;

	/** Type signature. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Def")
	FKzTypeDef Type;
};
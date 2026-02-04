// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
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
		return Name != NAME_None && ValueType != EPropertyBagPropertyType::None;
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
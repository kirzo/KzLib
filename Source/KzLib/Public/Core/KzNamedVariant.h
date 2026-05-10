// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzVariant.h"
#include "Core/KzParamDef.h"
#include "KzNamedVariant.generated.h"

/**
 * A named variant: an FName identifier paired with an FKzVariant value.
 * This is the unit that bridges KzVariant with named-property systems like FInstancedPropertyBag.
 *
 * Usage:
 *   FKzNamedVariant Health = FKzNamedVariant::Make<float>("Health", 100.0f);
 *   Health.WriteToBag(MyBag);
 *
 *   FKzNamedVariant Read = FKzNamedVariant::ReadFromBag(MyBag, "Health");
 *   if (Read.IsValid() && Read.GetValue().Is<float>()) { ... }
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzNamedVariant
{
	GENERATED_BODY()

public:
	static const FKzNamedVariant Invalid;

	FKzNamedVariant() = default;

	FKzNamedVariant(FName InName, const FKzVariant& InValue) : Name(InName), Value(InValue) {}

	/** Constructs a named variant from a name and a typed value. */
	template <typename T>
	static FKzNamedVariant Make(FName InName, const T& InValue)
	{
		return FKzNamedVariant(InName, FKzVariant::Make<T>(InValue));
	}

	/** True if the variant has a valid name and a valid value. */
	bool IsValid() const { return !Name.IsNone() && Value.IsValid(); }

	FName GetName() const { return Name; }
	void SetName(FName InName) { Name = InName; }

	const FKzVariant& GetValue() const { return Value; }
	FKzVariant& GetValue() { return Value; }
	void SetValue(const FKzVariant& InValue) { Value = InValue; }

	/** Builds a parameter definition matching this variant's name and value type. */
	FKzParamDef ToParamDef() const
	{
		return FKzParamDef(Name, EPropertyBagContainerType::None, Value.GetType(), Value.GetTypeObject());
	}

	/** Builds a PropertyBag descriptor matching this variant's name and value type. */
	FPropertyBagPropertyDesc ToPropertyDesc() const
	{
		return FPropertyBagPropertyDesc(Name, EPropertyBagContainerType::None, Value.GetType(), Value.GetTypeObject());
	}

	/** Adds (or replaces) this variant's value into the bag under its own name. Returns true on success. */
	bool WriteToBag(FInstancedPropertyBag& Bag) const;

	/** Reads a named value from the bag into a new named variant. Returns Invalid if the property doesn't exist. */
	static FKzNamedVariant ReadFromBag(const FInstancedPropertyBag& Bag, FName Name);

	bool operator==(const FKzNamedVariant& Other) const
	{
		return Name == Other.Name && Value == Other.Value;
	}
	bool operator!=(const FKzNamedVariant& Other) const { return !(*this == Other); }

private:
	UPROPERTY(EditAnywhere, Category = "KzNamedVariant")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, Category = "KzNamedVariant")
	FKzVariant Value;
};
// Copyright 2026 kirzo

#include "Core/KzNamedVariant.h"

const FKzNamedVariant FKzNamedVariant::Invalid = FKzNamedVariant();

bool FKzNamedVariant::WriteToBag(FInstancedPropertyBag& Bag) const
{
	if (!IsValid())
	{
		return false;
	}

	// Ensure the bag has a property with this name and the right type.
	// AddProperties replaces existing entries with the same name, so this is idempotent.
	Bag.AddProperties({ ToPropertyDesc() });

	const UPropertyBag* BagStruct = Bag.GetPropertyBagStruct();
	if (!BagStruct)
	{
		return false;
	}

	const FProperty* BagProp = BagStruct->FindPropertyByName(Name);
	if (!BagProp)
	{
		return false;
	}

	FStructView View = Bag.GetMutableValue();
	uint8* Memory = View.GetMemory();
	if (!Memory)
	{
		return false;
	}

	void* DestPtr = BagProp->ContainerPtrToValuePtr<void>(Memory);
	return Value.ToProperty(BagProp, DestPtr);
}

FKzNamedVariant FKzNamedVariant::ReadFromBag(const FInstancedPropertyBag& Bag, FName Name)
{
	const UPropertyBag* BagStruct = Bag.GetPropertyBagStruct();
	if (!BagStruct)
	{
		return FKzNamedVariant();
	}

	const FProperty* BagProp = BagStruct->FindPropertyByName(Name);
	if (!BagProp)
	{
		return FKzNamedVariant();
	}

	FConstStructView View = Bag.GetValue();
	const uint8* Memory = View.GetMemory();
	if (!Memory)
	{
		return FKzNamedVariant();
	}

	const void* SrcPtr = BagProp->ContainerPtrToValuePtr<void>(Memory);
	return FKzNamedVariant(Name, FKzVariant::FromProperty(BagProp, SrcPtr));
}
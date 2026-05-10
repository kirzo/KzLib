// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Algo/Transform.h"
#include "Core/KzParamDef.h"
#include "Concepts/KzIterable.h"
#include "StructUtils/PropertyBag.h"

/**
 * Operations on FInstancedPropertyBag that work with KzLib types (FKzParamDef, FKzTypeDef, FKzVariant, FKzNamedVariant).
 * For generic, KzLib-agnostic bag operations, see KzPropertyBagHelpers.
 */
namespace KzBagOps
{
	/** Adds (or replaces) a property in the bag from a single parameter definition. Skips invalid defs. */
	inline void AddProperty(FInstancedPropertyBag& Bag, const FKzParamDef& Def)
	{
		if (!Def.Name.IsNone() && Def.IsValid()) Bag.AddProperties({ Def.ToPropertyDesc() });
	}

	/** Adds (or replaces) multiple properties in the bag from any iterable of parameter definitions. Invalid defs are skipped. */
	inline void AddProperties(FInstancedPropertyBag& Bag, const CKzIterable auto& Defs)
	{
		TArray<FPropertyBagPropertyDesc> Descs;
		Algo::TransformIf(Defs, Descs,
			[](const FKzParamDef& Def) { return !Def.Name.IsNone() && Def.IsValid(); },
			[](const FKzParamDef& Def) { return Def.ToPropertyDesc(); });

		if (Descs.Num() > 0)
		{
			Bag.AddProperties(Descs);
		}
	}
}
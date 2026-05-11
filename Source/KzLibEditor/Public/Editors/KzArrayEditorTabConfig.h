// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FKzPropertyStackRowCustomizer;

/**
 * Describes a single array-backed tab inside a FKzArrayAssetEditor.
 *
 * The editor spawns one SKzPropertyStack per config, all sharing the same Element
 * Details panel on the right. When the user selects items in any tab, the details
 * panel reflects that tab's selection.
 *
 * The array may live directly on the asset (PropertyPath = { "MyArray" }) or
 * nested inside a struct (PropertyPath = { "Database", "Items" }).
 */
struct KZLIBEDITOR_API FKzArrayEditorTabConfig
{
	/** Path to the array property, walked from the asset root. Single-element for
	 *  direct properties, multi-element for arrays inside nested structs. */
	TArray<FName> PropertyPath;

	/** Singular noun used in UI: "Line", "Alias". */
	FText ItemName;

	/** Optional plural form. When empty, derived from ItemName via simple English rules
	 *  (adds "es" to words ending in s/x/z/ch/sh, "s" otherwise). */
	FText ItemNamePlural;

	TSharedPtr<FKzPropertyStackRowCustomizer> RowCustomizer;

	FKzArrayEditorTabConfig() = default;

	/** Convenience constructor for a direct array property on the asset. */
	FKzArrayEditorTabConfig(FName InArrayPropertyName, FText InItemName, TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: PropertyPath({ InArrayPropertyName })
		, ItemName(InItemName)
		, RowCustomizer(InRowCustomizer)
	{
	}

	/** Convenience constructor for a direct array property with an explicit plural form. */
	FKzArrayEditorTabConfig(FName InArrayPropertyName, FText InItemName, FText InItemNamePlural, TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: PropertyPath({ InArrayPropertyName })
		, ItemName(InItemName)
		, ItemNamePlural(InItemNamePlural)
		, RowCustomizer(InRowCustomizer)
	{
	}

	/** Constructor for nested array properties (e.g. {"Database", "Items"}). */
	FKzArrayEditorTabConfig(TArray<FName> InPropertyPath, FText InItemName, TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: PropertyPath(MoveTemp(InPropertyPath))
		, ItemName(InItemName)
		, RowCustomizer(InRowCustomizer)
	{
	}

	/** Constructor for nested array properties with an explicit plural form. */
	FKzArrayEditorTabConfig(TArray<FName> InPropertyPath, FText InItemName, FText InItemNamePlural, TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: PropertyPath(MoveTemp(InPropertyPath))
		, ItemName(InItemName)
		, ItemNamePlural(InItemNamePlural)
		, RowCustomizer(InRowCustomizer)
	{
	}

	/** Resolve the plural form: explicit when provided, smart fallback otherwise. */
	FText GetPluralItemName() const
	{
		if (!ItemNamePlural.IsEmpty()) { return ItemNamePlural; }

		const FString Singular = ItemName.ToString();
		if (Singular.IsEmpty()) { return ItemName; }

		const FString Lower = Singular.ToLower();
		const bool bAddEs =
			Lower.EndsWith(TEXT("s")) || Lower.EndsWith(TEXT("x")) || Lower.EndsWith(TEXT("z")) ||
			Lower.EndsWith(TEXT("ch")) || Lower.EndsWith(TEXT("sh"));

		return FText::FromString(Singular + (bAddEs ? TEXT("es") : TEXT("s")));
	}
};
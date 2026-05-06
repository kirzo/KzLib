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
 */
struct KZLIBEDITOR_API FKzArrayEditorTabConfig
{
	/** Name of the array property on the asset (e.g. GET_MEMBER_NAME_CHECKED(UMyAsset, MyArray)). */
	FName ArrayPropertyName;

	/** Singular noun used in UI: "Line", "Alias". */
	FText ItemName;

	/** Optional plural form. When empty, derived from ItemName via simple English rules
	 *  (adds "es" to words ending in s/x/z/ch/sh, "s" otherwise). */
	FText ItemNamePlural;

	TSharedPtr<FKzPropertyStackRowCustomizer> RowCustomizer;

	FKzArrayEditorTabConfig() = default;

	FKzArrayEditorTabConfig(FName InArrayPropertyName, FText InItemName,
		TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: ArrayPropertyName(InArrayPropertyName)
		, ItemName(InItemName)
		, RowCustomizer(InRowCustomizer)
	{
	}

	FKzArrayEditorTabConfig(FName InArrayPropertyName, FText InItemName, FText InItemNamePlural,
		TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: ArrayPropertyName(InArrayPropertyName)
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
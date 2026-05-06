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

	/** Singular noun used in UI: "Line", "Alias". The tab title pluralizes it. */
	FText ItemName;

	/** Optional row customizer driving leading/trailing widgets, display text, etc. */
	TSharedPtr<FKzPropertyStackRowCustomizer> RowCustomizer;

	FKzArrayEditorTabConfig() = default;

	FKzArrayEditorTabConfig(FName InArrayPropertyName, FText InItemName,
		TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer = nullptr)
		: ArrayPropertyName(InArrayPropertyName)
		, ItemName(InItemName)
		, RowCustomizer(InRowCustomizer)
	{
	}
};
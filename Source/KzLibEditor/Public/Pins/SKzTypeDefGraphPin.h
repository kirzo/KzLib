// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SGraphPin.h"
#include "Core/KzTypeDef.h"
#include "StructUtils/PropertyBag.h"

/**
 * Graph pin widget for FKzTypeDef pins.
 * Displays the same type picker as the details panel customization, so BlueprintCallable
 * functions that accept an FKzTypeDef parameter expose an inline dropdown directly on the node.
 */
class KZLIBEDITOR_API SKzTypeDefGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SKzTypeDefGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

private:
	EVisibility GetPickerVisibility() const;

	/** Reads the current pin default value as an FKzTypeDef, using a cached parse to avoid re-importing on every Slate paint. */
	const FKzTypeDef& ReadValue() const;

	/** Serializes the value back into the pin default through the schema (so undo/redo and BP modification flags work). */
	void WriteValue(const FKzTypeDef& NewValue);

	EPropertyBagPropertyType GetValueType() const;
	const UObject* GetValueTypeObject() const;
	EPropertyBagContainerType GetContainerType() const;

	void OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType);

	mutable FKzTypeDef CachedValue;
	mutable FString CachedRawString;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "StructUtils/PropertyBag.h"

struct FKzVariant;

/**
 * Customization for FKzVariant.
 * Shows a type selector and only the slot matching the current type.
 */
class FKzVariantCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzVariantCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** Reads the current variant from the handle. Returns nullptr if multi-edit or invalid. */
	const FKzVariant* GetVariant() const;

	/** Returns the current variant type, or None on multi-edit/invalid. */
	EPropertyBagPropertyType GetCurrentType() const;

	/** Returns the current variant type-object (UScriptStruct/UClass/UEnum), or nullptr. */
	const UObject* GetCurrentTypeObject() const;

	/** Called when the user picks a new type from SKzTypeSelector. */
	void OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType);

	/** Maps a property bag type to its slot UPROPERTY name. */
	static FName GetSlotPropertyName(EPropertyBagPropertyType Type);

	/** Rebuilds the SEnumComboBox content. Needed because SEnumComboBox takes the UEnum* by value, so we recreate it when the variant's UEnum changes. */
	void RebuildEnumComboBox();

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<class SBox> EnumComboHolder;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h" 
#include "StructUtils/PropertyBag.h"

struct FKzParamDef;

/**
 * Customization for FKzParamDef.
 * Renders as: [Name] [PinTypeSelector]
 */
class FKzParamDefCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() {return MakeShareable(new FKzParamDefCustomization); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** Returns the underlying FKzParamDef being edited, or nullptr on multi-edit. */
	const FKzParamDef* GetParamDef() const;

	/** Updates the struct when the Name text box changes */
	void OnNameChanged();

	/** Writes the new type info back into FKzParamDef::Type when the user picks a type. */
	void OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType);

	TSharedPtr<IPropertyHandle> StructHandle;
};
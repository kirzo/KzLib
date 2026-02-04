// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h" 

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
	TSharedPtr<IPropertyHandle> StructHandle;
	TAttribute<FEdGraphPinType> PinTypeAttribute;

	/** Updates the struct when the Name text box changes */
	void OnNameChanged();

	/** Helper to retrieve the current FKzParamDef value from the property handle. */
	FKzParamDef GetValue() const;

	/** Callback executed when the selector widget updates the definition. */
	void OnValueChanged(const FKzParamDef& NewDef);
};
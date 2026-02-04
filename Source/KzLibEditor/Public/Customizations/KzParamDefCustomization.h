// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h" 

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

	/** Gets the current PinType from the struct data */
	FEdGraphPinType GetTargetPinType() const;

	/** Updates the struct when the PinTypeSelector changes */
	void OnPinTypeChanged(const FEdGraphPinType& PinType);
};
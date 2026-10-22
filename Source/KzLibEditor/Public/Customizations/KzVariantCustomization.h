// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "StructUtils/PropertyBag.h"

class IPropertyUtilities;
struct FKzVariant;

/** Customization for FKzVariant: a type selector, and the value edited through the property of its type's bag struct. */
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
	/** The edited variant, or null on multi-edit. */
	const FKzVariant* GetVariant() const;

	void OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType);

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
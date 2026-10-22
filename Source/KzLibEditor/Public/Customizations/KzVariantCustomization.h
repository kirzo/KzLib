// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyUtilities;

/** Customization for FKzVariant: a single row with the type selector by the name and the value, edited through its type's bag struct, on the right. */
class FKzVariantCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzVariantCustomization);
	}

	/** Left empty, so the variant shows as its value row alone. */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	/** The variant's type selector, collapsed when its type is fixed. */
	static TSharedRef<SWidget> MakeTypeSelector(const TSharedRef<IPropertyHandle>& VariantHandle, const TSharedPtr<IPropertyUtilities>& PropertyUtilities);

	/** Adds the variant as one row: NameWidget on the left, the value on the right. Struct values keep their members as children. */
	static void AddVariantRow(const TSharedRef<IPropertyHandle>& VariantHandle, IDetailChildrenBuilder& ChildBuilder, const TSharedRef<SWidget>& NameWidget);
};
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

	/**
	 * Adds the variant as one row: NameWidget on the left, the value on the right, and struct values' members as children. RowHandle is the
	 * customized property the row stands for, whose array element menu the row shows when it is one.
	 */
	static void AddVariantRow(const TSharedRef<IPropertyHandle>& RowHandle, const TSharedRef<IPropertyHandle>& VariantHandle, IDetailChildrenBuilder& ChildBuilder, const TSharedRef<SWidget>& NameWidget);
};

/** Customization for FKzNamedVariant: the variant's row with a box for the name before the type selector. Array elements drop their index label. */
class FKzNamedVariantCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzNamedVariantCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};
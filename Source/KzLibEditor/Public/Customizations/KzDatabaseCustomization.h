// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "IPropertyTypeCustomization.h"

struct FKzDatabase;

/** Customization for FKzDatabaseItem: one row, an issue icon, the ID and the tags on the left and the value on the right, with the array element menu. */
class FKzDatabaseItemCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzDatabaseItemCustomization);
	}

	/** Left empty, so the item shows as its one row. */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};

/**
 * Customization for FKzDatabase.
 * The header holds the type selector and, unless HideItems, the row count and an add button, with the rows right under it.
 * Handles the auto-sync logic when Type changes.
 */
class FKzDatabaseCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzDatabaseCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	const FKzDatabase* GetDatabase() const;
	void OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType);
	void OnItemsArrayChanged();

	TSharedPtr<IPropertyHandle> StructHandle;
};
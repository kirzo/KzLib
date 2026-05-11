// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "IPropertyTypeCustomization.h"

struct FKzDatabase;

/**
 * Customization for FKzDatabaseItem.
 * Shows ID, Tags, and the generic Value inline.
 */
class FKzDatabaseItemCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FKzDatabaseItemCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};

/**
 * Customization for FKzDatabase.
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
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Layout/Visibility.h"

class IPropertyHandle;
class SComboButton;
class AActor;
class USceneComponent;

/**
 * Customization for FKzComponentSocketReference.
 * Provides dropdowns for Component selection (parsing SCS for blueprints) and Socket selection.
 */
class FKzComponentSocketReferenceCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	// -- Component Picking --
	TSharedRef<SWidget> OnGetComponentsMenu();
	void OnComponentSelected(FName InName);
	FText GetCurrentComponentName() const;

	// -- Socket Picking --
	TSharedRef<SWidget> OnGetSocketsMenu();
	void OnSocketSelected(FName InName);
	FText GetCurrentSocketName() const;
	EVisibility GetSocketVisibility() const;

	// -- Helpers --
	void BuildComponentList(TArray<FName>& OutNames);
	class USceneComponent* FindComponentByName(FName Name) const;

	/** Returns the OverrideActor if valid, otherwise attempts to return the owner Actor. */
	AActor* GetTargetActor() const;

	/** Callback when OverrideActor property value changes to reset invalid component names */
	void OnOverrideActorChanged();

	// -- Property Handles --
	bool bIsActorContext = false;
	bool bIsInstance = false;

	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> OverrideActorHandle;
	TSharedPtr<IPropertyHandle> ComponentNameHandle;
	TSharedPtr<IPropertyHandle> SocketNameHandle;
	TSharedPtr<IPropertyHandle> RelativeTransformHandle;
};
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
 * Customization for FKzComponentReference.
 * Provides dropdowns for Component selection (parsing SCS for blueprints) and Socket selection.
 */
class FKzComponentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/**
	 * Walks up the Outer chain of the provided Object to find the responsible Actor context.
	 * Handles: Direct Actors, Components, Instanced Objects, and Blueprint CDOs.
	 */
	AActor* ResolveActorContext(UObject* Obj) const;

	/** Helper to determine which Actor to inspect for components (Override vs Resolved Context). */
	AActor* GetTargetActor() const;

	/** Populates the array with valid UClass pointers. */
	void GetAllowedComponentClasses(TArray<UClass*>& OutClasses) const;
	void GetMustImplementInterfaces(TArray<UClass*>& OutInterfaces) const;

	// -- Component Picking --
	TSharedRef<SWidget> OnGetComponentsMenu();
	void OnComponentSelected(FName InName);
	FText GetCurrentComponentName() const;
	EVisibility GetComponentWarningVisibility() const;

	// -- Socket Picking --
	TSharedRef<SWidget> OnGetSocketsMenu();
	void OnSocketSelected(FName InName);
	FText GetCurrentSocketName() const;
	EVisibility GetSocketVisibility() const;
	EVisibility GetSocketWarningVisibility() const;

	// -- Helpers --
	void BuildComponentList(TArray<FName>& OutNames);
	USceneComponent* FindComponentByName(FName Name) const;
	void OnOverrideActorChanged();

	/**
	 * Helper that copies the entire struct, applies a modification, and writes it back as a full string.
	 * This prevents TSet hash corruption by forcing the Property System to do a full Remove+Add cycle.
	 */
	void ApplyFullStructUpdate(TFunctionRef<void(void*, UStruct*)> UpdateLogic);

	// -- Property Handles --
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> OverrideActorHandle;
	TSharedPtr<IPropertyHandle> ComponentNameHandle;
	TSharedPtr<IPropertyHandle> SocketNameHandle;
	TSharedPtr<IPropertyHandle> RelativeLocationHandle;
	TSharedPtr<IPropertyHandle> RelativeRotationHandle;
};
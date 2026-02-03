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
	/**
	 * Walks up the Outer chain of the provided Object to find the responsible Actor context.
	 * Handles: Direct Actors, Components, Instanced Objects, and Blueprint CDOs.
	 */
	AActor* ResolveActorContext(UObject* Obj) const;

	/** Helper to determine which Actor to inspect for components (Override vs Resolved Context). */
	AActor* GetTargetActor() const;

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
	USceneComponent* FindComponentByName(FName Name) const;
	void OnOverrideActorChanged();

	// -- Property Handles --
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> OverrideActorHandle;
	TSharedPtr<IPropertyHandle> ComponentNameHandle;
	TSharedPtr<IPropertyHandle> SocketNameHandle;
	TSharedPtr<IPropertyHandle> RelativeLocationHandle;
	TSharedPtr<IPropertyHandle> RelativeRotationHandle;
};
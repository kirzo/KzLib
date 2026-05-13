// Copyright 2026 kirzo

#pragma once

#include "KzLibEditor.h"
#include "KzLibEditorStyle.h"
#include "Editors/KzArrayAssetEditor.h"

#include "Core/KzTypeDef.h"
#include "Customizations/KzTypeDefCustomization.h"

#include "Core/KzVariant.h"
#include "Customizations/KzVariantCustomization.h"

#include "Core/KzParamDef.h"
#include "Customizations/KzParamDefCustomization.h"

#include "Core/KzDatabase.h"
#include "Customizations/KzDatabaseCustomization.h"

#include "Core/KzDatabaseAsset.h"

#include "Components/KzComponentReference.h"
#include "Customizations/KzComponentReferenceCustomization.h"

#include "Actors/KzActorGroup.h"
#include "ComponentVisualizers/KzActorGroupVisualizer.h"

#define LOCTEXT_NAMESPACE "FKzLibEditorModule"

void FKzLibEditorModule::OnStartupModule()
{
	FKzLibEditorStyle::Initialize();

	RegisterPropertyLayout<FKzTypeDef, FKzTypeDefCustomization>();
	RegisterPropertyLayout<FKzVariant, FKzVariantCustomization>();
	RegisterPropertyLayout<FKzParamDef, FKzParamDefCustomization>();
	RegisterPropertyLayout<FKzDatabase, FKzDatabaseCustomization>();
	RegisterPropertyLayout<FKzDatabaseItem, FKzDatabaseItemCustomization>();
	RegisterPropertyLayout<FKzComponentReference, FKzComponentReferenceCustomization>();
	RegisterPropertyLayout<FKzComponentSocketReference, FKzComponentReferenceCustomization>();

	RegisterComponentVisualizer<UKzActorGroupComponent, FKzActorGroupVisualizer>();

	TArray<FKzArrayEditorTabConfig> DatabaseTabs;
	DatabaseTabs.Add(FKzArrayEditorTabConfig(
		TArray<FName>{ GET_MEMBER_NAME_CHECKED(UKzDatabaseAsset, Database), GET_MEMBER_NAME_CHECKED(FKzDatabase, Items) },
		INVTEXT("Item")));

	RegisterAssetTypeAction<UKzDatabaseAsset, FKzArrayAssetEditor>(
		KzAssetCategoryBit,
		INVTEXT("Database"),
		FColor::FromHex("#3FF1B5"),
		TArray<FText>{},
		DatabaseTabs);
}

void FKzLibEditorModule::OnShutdownModule()
{
	FKzLibEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzLibEditorModule, KzLibEditor);
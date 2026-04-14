// Copyright 2026 kirzo

#pragma once

#include "KzLibEditor.h"
#include "KzLibEditorStyle.h"

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

	RegisterPropertyLayout<FKzParamDef, FKzParamDefCustomization>();
	RegisterPropertyLayout<FKzDatabase, FKzDatabaseCustomization>();
	RegisterPropertyLayout<FKzDatabaseItem, FKzDatabaseItemCustomization>();
	RegisterPropertyLayout<FKzComponentReference, FKzComponentReferenceCustomization>();
	RegisterPropertyLayout<FKzComponentSocketReference, FKzComponentReferenceCustomization>();

	RegisterComponentVisualizer<UKzActorGroupComponent, FKzActorGroupVisualizer>();

	RegisterAssetTypeAction<UKzDatabaseAsset>(KzAssetCategoryBit, INVTEXT("Database"), FColor::FromHex("#9B59B6"));
}

void FKzLibEditorModule::OnShutdownModule()
{
	FKzLibEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzLibEditorModule, KzLibEditor);
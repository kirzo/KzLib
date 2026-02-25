// Copyright 2026 kirzo

#pragma once

#include "KzLibEditor.h"

#include "Core/KzParamDef.h"
#include "Customizations/KzParamDefCustomization.h"

#include "Core/KzDatabase.h"
#include "Customizations/KzDatabaseCustomization.h"

#include "Components/KzComponentSocketReference.h"
#include "Customizations/KzComponentSocketReferenceCustomization.h"

#include "Actors/KzActorGroup.h"
#include "ComponentVisualizers/KzActorGroupVisualizer.h"

#define LOCTEXT_NAMESPACE "FKzLibEditorModule"

void FKzLibEditorModule::OnStartupModule()
{
	RegisterPropertyLayout<FKzParamDef, FKzParamDefCustomization>();
	RegisterPropertyLayout<FKzDatabase, FKzDatabaseCustomization>();
	RegisterPropertyLayout<FKzDatabaseItem, FKzDatabaseItemCustomization>();
	RegisterPropertyLayout<FKzComponentSocketReference, FKzComponentSocketReferenceCustomization>();

	RegisterComponentVisualizer<UKzActorGroupComponent, FKzActorGroupVisualizer>();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzLibEditorModule, KzLibEditor);
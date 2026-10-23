// Copyright 2026 kirzo

#pragma once

#include "KzLibEditor.h"
#include "KzLibEditorStyle.h"
#include "Editors/KzArrayAssetEditor.h"

#include "Core/KzTypeDef.h"
#include "Customizations/KzTypeDefCustomization.h"
#include "Pins/KzTypeDefPinFactory.h"

#include "Core/KzVariant.h"
#include "Core/KzNamedVariant.h"
#include "Customizations/KzVariantCustomization.h"

#include "Core/KzParamDef.h"
#include "Customizations/KzParamDefCustomization.h"

#include "Core/KzDatabase.h"
#include "Customizations/KzDatabaseCustomization.h"

#include "Core/KzDatabaseAsset.h"
#include "Widgets/SKzDatabaseQueryTester.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#include "Components/KzComponentReference.h"
#include "Customizations/KzComponentReferenceCustomization.h"

#include "Actors/KzActorGroup.h"
#include "ComponentVisualizers/KzActorGroupVisualizer.h"

#define LOCTEXT_NAMESPACE "FKzLibEditorModule"

void FKzLibEditorModule::OnStartupModule()
{
	FKzLibEditorStyle::Initialize();

	RegisterPropertyLayout<FKzTypeDef, FKzTypeDefCustomization>();
	RegisterPinFactory<FKzTypeDefPinFactory>();

	RegisterPropertyLayout<FKzVariant, FKzVariantCustomization>();
	RegisterPropertyLayout<FKzNamedVariant, FKzNamedVariantCustomization>();
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

	TArray<FKzCustomEditorTabConfig> DatabaseCustomTabs;
	{
		FKzCustomEditorTabConfig& QueryTab = DatabaseCustomTabs.AddDefaulted_GetRef();
		QueryTab.TabId = TEXT("KzDatabase_QueryTest");
		QueryTab.Label = INVTEXT("Query Test");
		QueryTab.IconStyleName = TEXT("Icons.Search");
		QueryTab.MakeWidget = [](UObject* Asset) -> TSharedRef<SWidget>
		{
			return SNew(SKzDatabaseQueryTester).Object(Asset);
		};
	}

	RegisterAssetTypeAction<UKzDatabaseAsset, FKzArrayAssetEditor>(
		KzAssetCategoryBit,
		INVTEXT("Database"),
		FColor::FromHex("#3FF1B5"),
		TArray<FText>{},
		DatabaseTabs,
		DatabaseCustomTabs);

	// The same tester for any object and user, such as an item against a character Blueprint
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SKzDatabaseQueryTester::TabId, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
		{
			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				[
					SNew(SKzDatabaseQueryTester)
				];
		}))
		.SetDisplayName(LOCTEXT("QueryTestTab", "Database Query Test"))
		.SetTooltipText(LOCTEXT("QueryTestTabTooltip", "Try a database query about an object made by a user, and see which row wins and why the others lose."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FKzLibEditorStyle::Get().GetStyleSetName(), "ClassIcon.KzDatabaseAsset"));
}

void FKzLibEditorModule::OnShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SKzDatabaseQueryTester::TabId);
	}
	FKzLibEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzLibEditorModule, KzLibEditor);
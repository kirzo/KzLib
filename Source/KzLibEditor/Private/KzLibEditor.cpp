// Copyright 2026 kirzo

#pragma once

#include "KzLibEditor.h"
#include "AssetToolsModule.h"
#include "AssetTools/KzAssetTypeActions_Base.h"

#include "Core/KzParamDef.h"
#include "Customizations/KzParamDefCustomization.h"

#include "Core/KzDatabase.h"
#include "Customizations/KzDatabaseCustomization.h"

#include "Components/KzComponentSocketReference.h"
#include "Customizations/KzComponentSocketReferenceCustomization.h"

#include "ComponentVisualizers.h"

#include "Actors/KzActorGroup.h"
#include "ComponentVisualizers/KzActorGroupVisualizer.h"

#define LOCTEXT_NAMESPACE "FKzLibEditorModule"

void FKzLibEditorModule::StartupModule()
{
	RegisterAssetTools();
	RegisterLayouts();
	RegisterComponentVisualizers();
}

void FKzLibEditorModule::ShutdownModule()
{
	UnregisterAssetTools();
	UnregisterLayouts();
}

template<typename T>
void FKzLibEditorModule::RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto Action = MakeShared<T>(AssetCategory);
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

template<typename T>
void FKzLibEditorModule::RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto Action = MakeShared<FKzAssetTypeActions>(AssetCategory, Name, Color, T::StaticClass());
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

template<typename T>
void FKzLibEditorModule::RegisterClassLayout(const FName ClassName)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	RegisteredClassLayouts.AddUnique(ClassName);
	PropertyEditorModule.RegisterCustomClassLayout(ClassName, FOnGetDetailCustomizationInstance::CreateStatic(&T::MakeInstance));
}

template<typename T>
void FKzLibEditorModule::RegisterPropertyLayout(const FName TypeName)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	RegisteredPropertyLayouts.AddUnique(TypeName);
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(TypeName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&T::MakeInstance));
}

void FKzLibEditorModule::RegisterAssetTools()
{
}

void FKzLibEditorModule::RegisterLayouts()
{
	RegisterPropertyLayout<FKzParamDefCustomization>(FKzParamDef::StaticStruct()->GetFName());
	RegisterPropertyLayout<FKzDatabaseCustomization>(FKzDatabase::StaticStruct()->GetFName());
	RegisterPropertyLayout<FKzDatabaseItemCustomization>(FKzDatabaseItem::StaticStruct()->GetFName());
	RegisterPropertyLayout<FKzComponentSocketReferenceCustomization>(FKzComponentSocketReference::StaticStruct()->GetFName());
}

void FKzLibEditorModule::RegisterComponentVisualizers()
{
	FComponentVisualizersModule& ComponentVisualizersModule = FModuleManager::LoadModuleChecked<FComponentVisualizersModule >("ComponentVisualizers");
	ComponentVisualizersModule.RegisterComponentVisualizer(UKzActorGroupComponent::StaticClass()->GetFName(), MakeShareable(new FKzActorGroupVisualizer));
}

void FKzLibEditorModule::UnregisterAssetTools()
{
	if (auto* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools"))
	{
		IAssetTools& AssetTools = AssetToolsModule->Get();
		for (auto& Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}
	}
}

void FKzLibEditorModule::UnregisterLayouts()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		for (const FName ClassName : RegisteredClassLayouts)
		{
			PropertyEditorModule.UnregisterCustomClassLayout(ClassName);
		}

		for (const FName TypeName : RegisteredPropertyLayouts)
		{
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TypeName);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKzLibEditorModule, KzLibEditor);
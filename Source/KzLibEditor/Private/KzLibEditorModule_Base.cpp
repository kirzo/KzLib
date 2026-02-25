// Copyright 2026 kirzo

#include "KzLibEditorModule_Base.h"

#define LOCTEXT_NAMESPACE "FKzLibEditorModule_Base"

void FKzLibEditorModule_Base::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	KzAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory("KzLib", INVTEXT("KzLib"));

	OnStartupModule();
}

void FKzLibEditorModule_Base::ShutdownModule()
{
	UnregisterAssetTools();
	UnregisterLayouts();

	OnShutdownModule();
}

void FKzLibEditorModule_Base::UnregisterAssetTools()
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

void FKzLibEditorModule_Base::UnregisterLayouts()
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
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FKzLibEditorModule : public IModuleInterface
{
private:
	TArray<TSharedRef<class IAssetTypeActions>> RegisteredAssetTypeActions;
	TArray<FName> RegisteredClassLayouts;
	TArray<FName> RegisteredPropertyLayouts;

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	template<typename T>
	void RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory);

	template<typename T>
	void RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color);

	template<typename T>
	void RegisterClassLayout(const FName ClassName);

	template<typename T>
	void RegisterPropertyLayout(const FName TypeName);

protected:
	virtual void RegisterAssetTools();
	virtual void RegisterLayouts();
	virtual void RegisterComponentVisualizers();

private:
	void UnregisterAssetTools();
	void UnregisterLayouts();
};
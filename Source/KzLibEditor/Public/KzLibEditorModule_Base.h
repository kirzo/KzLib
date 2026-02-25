// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

#include "PropertyEditorModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ComponentVisualizers.h"

#include "AssetTools/KzAssetTypeActions_Base.h"

class KZLIBEDITOR_API FKzLibEditorModule_Base : public IModuleInterface
{
protected:
	EAssetTypeCategories::Type KzAssetCategoryBit = EAssetTypeCategories::None;

private:
	TArray<TSharedRef<class IAssetTypeActions>> RegisteredAssetTypeActions;
	TArray<FName> RegisteredClassLayouts;
	TArray<FName> RegisteredPropertyLayouts;
	TArray<FName> RegisteredComponentVisualizers;

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override final;
	virtual void ShutdownModule() override final;

	template<typename T>
	void RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory);

	template<typename T>
	void RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color, const TArray<FText>& SubMenus = TArray<FText>());

	template<typename TType, typename TCustomization>
	void RegisterPropertyLayout();

	template<typename TClass, typename TCustomization>
	void RegisterClassLayout();

	template<typename TComponent, typename TVisualizer>
	void RegisterComponentVisualizer();

protected:
	virtual void OnStartupModule() {}
	virtual void OnShutdownModule() {}

private:
	void UnregisterAssetTools();
	void UnregisterLayouts();
};

template<typename T>
void FKzLibEditorModule_Base::RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto Action = MakeShared<T>(AssetCategory);
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

template<typename T>
void FKzLibEditorModule_Base::RegisterAssetTypeAction(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color, const TArray<FText>& SubMenus)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const auto Action = MakeShared<FKzAssetTypeActions>(AssetCategory, Name, Color, T::StaticClass(), SubMenus);
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

template<typename TType, typename TCustomization>
void FKzLibEditorModule_Base::RegisterPropertyLayout()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FName TypeName;

	if constexpr (std::is_base_of_v<UObject, TType>)
	{
		TypeName = TType::StaticClass()->GetFName();
	}
	else
	{
		TypeName = TType::StaticStruct()->GetFName();
	}

	RegisteredPropertyLayouts.AddUnique(TypeName);
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(TypeName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&TCustomization::MakeInstance));
}

template<typename TClass, typename TCustomization>
void FKzLibEditorModule_Base::RegisterClassLayout()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	const FName ClassName = TClass::StaticClass()->GetFName();

	RegisteredClassLayouts.AddUnique(ClassName);
	PropertyEditorModule.RegisterCustomClassLayout(ClassName, FOnGetDetailCustomizationInstance::CreateStatic(&TCustomization::MakeInstance));
}

template<typename TComponent, typename TVisualizer>
void FKzLibEditorModule_Base::RegisterComponentVisualizer()
{
	FComponentVisualizersModule& ComponentVisualizersModule = FModuleManager::LoadModuleChecked<FComponentVisualizersModule>("ComponentVisualizers");

	const FName ComponentName = TComponent::StaticClass()->GetFName();

	ComponentVisualizersModule.RegisterComponentVisualizer(ComponentName, MakeShareable(new TVisualizer()));
	RegisteredComponentVisualizers.AddUnique(ComponentName);
}
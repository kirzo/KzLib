// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SKzPropertyStack;
class IPropertyHandle;

class KZLIBEDITOR_API FKzArrayAssetEditor : public FAssetEditorToolkit
{
	friend class FKzArrayAssetDetailCustomization;

public:
	static TSharedRef<FKzArrayAssetEditor> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit, FName InArrayPropertyName, FText InItemName);

	void InitArrayAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset, FName InArrayPropertyName, FText InItemName);

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	TSharedRef<SDockTab> SpawnTab_AssetDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_ArrayStack(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_ElementDetails(const FSpawnTabArgs& Args);

	UObject* AssetToEdit = nullptr;
	FName ArrayPropertyName;
	FText ItemName;

	TSharedPtr<IDetailsView> AssetDetailsView;
	TSharedPtr<IDetailsView> ElementDetailsView;

	TSharedPtr<IPropertyHandle> ArrayPropertyHandle;
	TSharedPtr<SKzPropertyStack> PropertyStackWidget;

	static const FName AssetDetailsTabId;
	static const FName ArrayStackTabId;
	static const FName ElementDetailsTabId;

	void OnElementSelected(TSharedPtr<IPropertyHandle> SelectedHandle);
};
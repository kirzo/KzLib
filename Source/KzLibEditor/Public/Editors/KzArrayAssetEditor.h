// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editors/KzArrayEditorTabConfig.h"

class IDetailsView;
class SKzPropertyStack;
class IPropertyHandle;
class SBox;

/**
 * Generic asset editor that displays one or more array-backed tabs (each a
 * SKzPropertyStack) plus a shared Element Details panel and a Validation panel.
 *
 * Tabs are configured via FKzArrayEditorTabConfig. Customizers per tab decide how
 * each row is rendered (display text, icons, action buttons).
 */
class KZLIBEDITOR_API FKzArrayAssetEditor : public FAssetEditorToolkit
{
	friend class FKzArrayAssetDetailCustomization;

public:
	static TSharedRef<FKzArrayAssetEditor> CreateEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		const TArray<UObject*>& ObjectsToEdit,
		const TArray<FKzArrayEditorTabConfig>& InTabs);

	void InitArrayAssetEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UObject* InAsset,
		const TArray<FKzArrayEditorTabConfig>& InTabs);

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void OnClose() override;

	/** Accessor for customizers that need to inspect the asset (e.g. to look up
	 *  cross-tab data like resolving an alias's lines). */
	UObject* GetEditedAsset() const { return AssetToEdit; }

private:
	UObject* AssetToEdit = nullptr;
	TArray<FKzArrayEditorTabConfig> Tabs;

	TSharedPtr<IDetailsView> AssetDetailsView;
	TSharedPtr<IDetailsView> ElementDetailsView;
	TSharedPtr<SBox> ElementDetailsContainer;

	/** Per-tab runtime state. */
	struct FTabRuntime
	{
		FName ArrayPropertyName;
		FText ItemName;
		TSharedPtr<FKzPropertyStackRowCustomizer> Customizer;

		TSharedPtr<IPropertyHandle> ArrayPropertyHandle;
		TSharedPtr<SKzPropertyStack> StackWidget;

		FName TabId;
	};
	TArray<FTabRuntime> TabRuntimes;

	/** Validation panel (still global to the asset). */
	TSharedPtr<class SKzValidationPanel> ValidationPanel;

	static const FName AssetDetailsTabId;
	static const FName ValidationTabId;

	TSharedRef<SDockTab> SpawnTab_AssetDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_ElementDetails(const FSpawnTabArgs& Args, FName ElementDetailsTabId);
	TSharedRef<SDockTab> SpawnTab_ArrayStack(const FSpawnTabArgs& Args, int32 TabIndex);
	TSharedRef<SDockTab> SpawnTab_Validation(const FSpawnTabArgs& Args);

	void OnElementsSelected(const TArray<TSharedPtr<IPropertyHandle>>& SelectedHandles);
	void OnArrayStackTabActivated(TSharedRef<SDockTab> ActivatedTab, ETabActivationCause Cause, int32 TabIndex);

	void OnRunValidation();
	TArray<struct FKzValidationIssue> HandleRunValidation();
	void HandleValidationIssueActivated(const struct FKzValidationIssue& Issue);

	void ExtendToolbar();

	/** Returns "Element Details" tab id (singular per editor; only one details panel). */
	static FName GetElementDetailsTabId() { return TEXT("KzArrayEditor_ElementDetails"); }

	/** Returns the unique tab id for a given array stack tab. */
	static FName MakeArrayStackTabId(int32 Index)
	{
		return FName(*FString::Printf(TEXT("KzArrayEditor_ArrayStack_%d"), Index));
	}
};
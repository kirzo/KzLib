// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/SKzPropertyStack.h"

class IDetailsView;
class SKzPropertyStack;
class IPropertyHandle;

class KZLIBEDITOR_API FKzArrayAssetEditor : public FAssetEditorToolkit
{
	friend class FKzArrayAssetDetailCustomization;

public:
	static TSharedRef<FKzArrayAssetEditor> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit, FName InArrayPropertyName, FText InItemName, SKzPropertyStack::FOnGetItemDisplayName InOnGetItemDisplayName = SKzPropertyStack::FOnGetItemDisplayName());

	/**
	 * Creates a type-safe display name delegate for the property stack.
	 * Automatically handles the memory extraction and casting from the IPropertyHandle.
	 *
	 * @param Lambda A function/lambda taking a pointer to your concrete type (TElement*) and returning an FString.
	 */
	template<typename TElement, typename FCallable>
	static SKzPropertyStack::FOnGetItemDisplayName MakeDisplayDelegate(FCallable&& Lambda)
	{
		return SKzPropertyStack::FOnGetItemDisplayName::CreateLambda(
			[Func = Forward<FCallable>(Lambda)](TSharedPtr<IPropertyHandle> Handle) -> FString
			{
				if (!Handle.IsValid()) return TEXT("Invalid");

				if constexpr (TIsDerivedFrom<TElement, UObject>::IsDerived)
				{
					// For arrays of UObjects
					UObject* Obj = nullptr;
					if (Handle->GetValue(Obj) == FPropertyAccess::Success && Obj)
					{
						return Func(CastChecked<TElement>(Obj));
					}
				}
				else
				{
					// For arrays of UStructs or primitives
					void* RawData = nullptr;
					if (Handle->GetValueData(RawData) == FPropertyAccess::Success && RawData)
					{
						return Func(static_cast<TElement*>(RawData));
					}
				}

				return TEXT("Empty");
			});
	}

	void InitArrayAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset, FName InArrayPropertyName, FText InItemName, SKzPropertyStack::FOnGetItemDisplayName InOnGetItemDisplayName);

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

	SKzPropertyStack::FOnGetItemDisplayName OnGetItemDisplayNameDelegate;

	TSharedPtr<IDetailsView> AssetDetailsView;
	TSharedPtr<IDetailsView> ElementDetailsView;
	TSharedPtr<class SBox> ElementDetailsContainer;

	TSharedPtr<IPropertyHandle> ArrayPropertyHandle;
	TSharedPtr<SKzPropertyStack> PropertyStackWidget;

	static const FName AssetDetailsTabId;
	static const FName ArrayStackTabId;
	static const FName ElementDetailsTabId;

	void OnElementSelected(TSharedPtr<IPropertyHandle> SelectedHandle);
};
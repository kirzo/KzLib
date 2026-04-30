// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Toolkits/SimpleAssetEditor.h"
#include "Templates/Tuple.h"

class FKzAssetTypeActions_Base : public FAssetTypeActions_Base
{
public:
	explicit FKzAssetTypeActions_Base(EAssetTypeCategories::Type AssetCategory)
		: AssetCategory(AssetCategory)
	{
	}

	virtual uint32 GetCategories() override { return AssetCategory; }
	
private:
	EAssetTypeCategories::Type AssetCategory;
};

template<typename TEditor = FSimpleAssetEditor, typename... TArgs>
class TKzAssetTypeActions : public FKzAssetTypeActions_Base
{
public:
	const FText Name;
	const FColor Color;
	UClass* const SupportedClass;
	const TArray<FText> SubMenus;

	// Store the variadic arguments directly in an Tuple (Zero overhead, no heap allocations)
	TTuple<TArgs...> ExtraArgs;

	TKzAssetTypeActions(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color, UClass* SupportedClass, const TArray<FText>& SubMenus = TArray<FText>(), TArgs... Args)
		: FKzAssetTypeActions_Base(AssetCategory)
		, Name(Name)
		, Color(Color)
		, SupportedClass(SupportedClass)
		, SubMenus(SubMenus)
		, ExtraArgs(Forward<TArgs>(Args)...)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return SupportedClass; }
	virtual const TArray<FText>& GetSubMenus() const override { return SubMenus; }

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override
	{
		// Generate a compile-time sequence of indices (0, 1, 2...) based on the number of extra arguments
		OpenAssetEditorImpl(InObjects, EditWithinLevelEditor, TMakeIntegerSequence<uint32, sizeof...(TArgs)>());
	}

private:
	// Helper function to unpack the tuple into the function call
	template<uint32... Indices>
	void OpenAssetEditorImpl(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor, TIntegerSequence<uint32, Indices...>)
	{
		// The compiler expands ExtraArgs.template Get<Indices>()... to ExtraArgs.Get<0>(), ExtraArgs.Get<1>(), etc.
		// If TArgs is empty, it simply expands to nothing.
		TEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects, ExtraArgs.template Get<Indices>()...);
	}
};
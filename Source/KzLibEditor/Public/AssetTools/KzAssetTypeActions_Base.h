// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Toolkits/SimpleAssetEditor.h"

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

template<typename TEditor = FSimpleAssetEditor>
class TKzAssetTypeActions : public FKzAssetTypeActions_Base
{
public:
	const FText Name;
	const FColor Color;
	UClass* const SupportedClass;
	const TArray<FText> SubMenus;

	TKzAssetTypeActions(EAssetTypeCategories::Type AssetCategory, const FText& Name, FColor Color, UClass* SupportedClass, const TArray<FText>& SubMenus = TArray<FText>())
		: FKzAssetTypeActions_Base(AssetCategory)
		, Name(Name)
		, Color(Color)
		, SupportedClass(SupportedClass)
		, SubMenus(SubMenus)
	{
	}

	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return SupportedClass; }
	virtual const TArray<FText>& GetSubMenus() const override { return SubMenus; }

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override
	{
		TEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
	}
};
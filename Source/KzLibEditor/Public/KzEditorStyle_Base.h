// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"

/**
 * Base template for Slate Styles in Kz plugins.
 * Automatically handles Singleton initialization and provides helpers for standard editor icons.
 */
template <typename T>
class TKzEditorStyle_Base : public FSlateStyleSet
{
public:
	TKzEditorStyle_Base(const FName& InStyleSetName)
		: FSlateStyleSet(InStyleSetName)
	{
	}

	virtual ~TKzEditorStyle_Base() {}

	static void Initialize()
	{
		if (!StyleInstance.IsValid())
		{
			StyleInstance = MakeShared<T>();
			FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
		}
	}

	static void Shutdown()
	{
		if (StyleInstance.IsValid())
		{
			FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
			StyleInstance.Reset();
		}
	}

	static T& Get()
	{
		check(StyleInstance.IsValid());
		return *StyleInstance;
	}

protected:
	static TSharedPtr<T> StyleInstance;

	/** Sets the content root to the Resources folder of the specified plugin */
	void SetupPluginResources(const FString& PluginName)
	{
		const FString ResourcesDir = IPluginManager::Get().FindPlugin(PluginName)->GetBaseDir() / TEXT("Resources");
		SetContentRoot(ResourcesDir);
		SetCoreContentRoot(ResourcesDir);
	}

	/** Helper to add a 16x16 Class Icon */
	void AddClassIcon(const FName& ClassName, const FString& IconName)
	{
		const FString Path = RootToContentDir(IconName, TEXT(".png"));
		Set(*FString::Printf(TEXT("ClassIcon.%s"), *ClassName.ToString()), new FSlateImageBrush(Path, FVector2D(16.0f, 16.0f)));
	}

	/** Helper to add a 64x64 Class Thumbnail */
	void AddClassThumbnail(const FName& ClassName, const FString& IconName)
	{
		const FString Path = RootToContentDir(IconName, TEXT(".png"));
		Set(*FString::Printf(TEXT("ClassThumbnail.%s"), *ClassName.ToString()), new FSlateImageBrush(Path, FVector2D(64.0f, 64.0f)));
	}

	/** Helper to add a generic icon with a custom size */
	void AddIcon(const FName& PropertyName, const FString& IconName, const FVector2D& Size)
	{
		const FString Path = RootToContentDir(IconName, TEXT(".png"));
		Set(PropertyName, new FSlateImageBrush(Path, Size));
	}
};

// Initialize the static pointer
template <typename T>
TSharedPtr<T> TKzEditorStyle_Base<T>::StyleInstance = nullptr;
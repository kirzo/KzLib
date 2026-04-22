// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzAppLibrary.generated.h"

/** General-purpose App/Project utilities. */
UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzAppLibrary"))
class KZLIB_API UKzAppLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// === Project ===

	/** Returns the current project version as defined in Project Settings. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Project")
	static FString GetProjectVersion();

	/** Checks if the game is running as a final packaged build or inside the Unreal Editor. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Package")
	static bool IsPackagedBuild();
};
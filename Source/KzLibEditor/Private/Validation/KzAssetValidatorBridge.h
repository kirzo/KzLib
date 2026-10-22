// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "KzAssetValidatorBridge.generated.h"

/** Runs an asset's UKzAssetValidators through the engine's data validation, so they also report on save, in Validate Assets and in CI. */
UCLASS()
class UKzAssetValidatorBridge : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	UKzAssetValidatorBridge();

protected:
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
};
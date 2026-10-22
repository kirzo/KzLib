// Copyright 2026 kirzo

#include "Validation/KzAssetValidatorBridge.h"
#include "Validation/KzAssetValidationUtils.h"

UKzAssetValidatorBridge::UKzAssetValidatorBridge()
{
	bIsEnabled = true;
}

bool UKzAssetValidatorBridge::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return !FKzAssetValidationUtils::GatherValidatorsFor(InAsset).IsEmpty();
}

EDataValidationResult UKzAssetValidatorBridge::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	return FKzAssetValidationUtils::ReportToContext(InAssetData, FKzAssetValidationUtils::RunValidation(InAsset), InContext);
}
// Copyright 2026 kirzo

#include "Validation/KzAssetValidatorBridge.h"
#include "Validation/KzAssetValidationUtils.h"
#include "Misc/DataValidation.h"

UKzAssetValidatorBridge::UKzAssetValidatorBridge()
{
	bIsEnabled = true;
}

bool UKzAssetValidatorBridge::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return !FKzAssetValidationUtils::GatherValidatorsFor(InAsset, InContext.GetValidationUsecase() == EDataValidationUsecase::Save).IsEmpty();
}

EDataValidationResult UKzAssetValidatorBridge::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	const bool bSaving = InContext.GetValidationUsecase() == EDataValidationUsecase::Save;
	return FKzAssetValidationUtils::ReportToContext(InAssetData, FKzAssetValidationUtils::RunValidation(InAsset, bSaving), InContext);
}
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzGraphValidator.generated.h"

/** Runs a compile of a graph asset without touching it, to report what the compile would find. */
UCLASS()
class UKzGraphValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual FName GetValidatorId_Implementation() const override;
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};
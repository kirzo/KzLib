// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzDatabaseAssetValidator.generated.h"

/** Reports database tables that won't answer as meant: no type, rows of another type or without a value, repeated IDs, rows another always beats, and a parent of another type or in a loop. */
UCLASS()
class UKzDatabaseAssetValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};
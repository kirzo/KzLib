// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Validation/KzAssetValidator.h"
#include "KzDatabaseAssetValidator.generated.h"

/** Reports database tables that won't answer as meant: the table checks of KzDatabaseValidation, and a parent of another type or in a loop. */
UCLASS()
class UKzDatabaseAssetValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};

/** Runs the same table checks on the inline tables of Blueprints and data assets, such as a database component's or an item's Database fragment. */
UCLASS()
class UKzDatabaseInlineValidator : public UKzAssetValidator
{
	GENERATED_BODY()

public:
	virtual bool CanValidate_Implementation(const UObject* Asset) const override;
	virtual void Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const override;
};
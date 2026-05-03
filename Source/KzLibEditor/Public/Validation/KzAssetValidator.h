// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/KzValidationTypes.h"
#include "KzAssetValidator.generated.h"

/**
 * Abstract validator that inspects an asset and reports issues.
 *
 * Subclasses implement CanValidate to declare which asset types they handle, and
 * Validate to walk the asset and emit FKzValidationIssue instances.
 *
 * The validation system discovers all subclasses at runtime (GetDerivedClasses)
 * and runs every validator that returns true from CanValidate(Asset). Any plugin
 * can add validators without modifying the host editor.
 */
UCLASS(Abstract, Blueprintable)
class KZLIBEDITOR_API UKzAssetValidator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Stable identifier for this validator. Surfaced in the UI as a tag.
	 * Defaults to the class name; override only if you want a friendlier label.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Validation")
	FName GetValidatorId() const;
	virtual FName GetValidatorId_Implementation() const { return GetClass()->GetFName(); }

	/**
	 * Whether this validator applies to the given asset.
	 * The check is by-reference so subclasses can use IsA<>.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Validation")
	bool CanValidate(const UObject* Asset) const;
	virtual bool CanValidate_Implementation(const UObject* Asset) const { return false; }

	/** Run the validation pass and append any issues found to OutIssues. */
	UFUNCTION(BlueprintNativeEvent, Category = "Validation")
	void Validate(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const;
	virtual void Validate_Implementation(const UObject* /*Asset*/, TArray<FKzValidationIssue>& /*OutIssues*/) const {}
};
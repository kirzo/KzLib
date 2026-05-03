// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"

class UKzAssetValidator;

/** Utilities for discovering and running validators against an asset. */
class KZLIBEDITOR_API FKzAssetValidationUtils
{
public:
	/**
	 * Walk all loaded UClass children of UKzAssetValidator, instantiate one of each
	 * concrete (non-abstract) class via the CDO, and ask CanValidate. Returns the
	 * CDOs of the matching validators (no instances are spawned; CDO is sufficient
	 * because validators are stateless).
	 */
	static TArray<const UKzAssetValidator*> GatherValidatorsFor(const UObject* Asset);

	/** Run all matching validators against the asset and aggregate the issues. */
	static TArray<FKzValidationIssue> RunValidation(const UObject* Asset);
};
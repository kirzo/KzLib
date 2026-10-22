// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"

class FDataValidationContext;
class UKzAssetValidator;
enum class EDataValidationResult : uint8;
struct FAssetData;

/** Utilities for discovering and running validators against an asset. */
class KZLIBEDITOR_API FKzAssetValidationUtils
{
public:
	/** The CDOs of every concrete UKzAssetValidator whose CanValidate accepts Asset. Validators are stateless, so the CDO is enough. */
	static TArray<const UKzAssetValidator*> GatherValidatorsFor(const UObject* Asset);

	/** Run all matching validators against the asset and aggregate the issues. */
	static TArray<FKzValidationIssue> RunValidation(const UObject* Asset);

	/** Forwards Issues to the engine's data validation as messages on AssetData. Invalid only when one of them is an error. */
	static EDataValidationResult ReportToContext(const FAssetData& AssetData, const TArray<FKzValidationIssue>& Issues, FDataValidationContext& Context);
};
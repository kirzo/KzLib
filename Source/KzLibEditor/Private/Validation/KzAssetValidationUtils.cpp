// Copyright 2026 kirzo

#include "Validation/KzAssetValidationUtils.h"
#include "Validation/KzAssetValidator.h"
#include "UObject/UObjectIterator.h"

TArray<const UKzAssetValidator*> FKzAssetValidationUtils::GatherValidatorsFor(const UObject* Asset)
{
	TArray<const UKzAssetValidator*> Result;
	if (!IsValid(Asset)) { return Result; }

	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (!Class->IsChildOf(UKzAssetValidator::StaticClass())) { continue; }
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) { continue; }

		// Skip transient skeleton/reinstancer leftovers from BP recompiles.
		const FString ClassName = Class->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_"))) { continue; }

		const UKzAssetValidator* CDO = Cast<UKzAssetValidator>(Class->GetDefaultObject());
		if (CDO && CDO->CanValidate(Asset))
		{
			Result.Add(CDO);
		}
	}
	return Result;
}

TArray<FKzValidationIssue> FKzAssetValidationUtils::RunValidation(const UObject* Asset)
{
	TArray<FKzValidationIssue> Issues;
	for (const UKzAssetValidator* Validator : GatherValidatorsFor(Asset))
	{
		Validator->Validate(Asset, Issues);
	}
	return Issues;
}
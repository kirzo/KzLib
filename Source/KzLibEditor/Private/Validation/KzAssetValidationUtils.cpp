// Copyright 2026 kirzo

#include "Validation/KzAssetValidationUtils.h"
#include "Validation/KzAssetValidator.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/DataValidation.h"
#include "UObject/UObjectHash.h"

TArray<const UKzAssetValidator*> FKzAssetValidationUtils::GatherValidatorsFor(const UObject* Asset)
{
	TArray<const UKzAssetValidator*> Result;
	if (!IsValid(Asset)) { return Result; }

	// The class tree's derived-class map, not a walk over every loaded UClass.
	TArray<UClass*> Classes;
	GetDerivedClasses(UKzAssetValidator::StaticClass(), Classes);
	for (UClass* Class : Classes)
	{
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

EDataValidationResult FKzAssetValidationUtils::ReportToContext(const FAssetData& AssetData, const TArray<FKzValidationIssue>& Issues, FDataValidationContext& Context)
{
	bool bHasErrors = false;
	for (const FKzValidationIssue& Issue : Issues)
	{
		const EMessageSeverity::Type Severity =
			Issue.Severity == EKzValidationSeverity::Error ? EMessageSeverity::Error :
			Issue.Severity == EKzValidationSeverity::Warning ? EMessageSeverity::Warning :
			EMessageSeverity::Info;
		Context.AddMessage(AssetData, Severity, Issue.Message);
		bHasErrors |= Issue.Severity == EKzValidationSeverity::Error;
	}
	return bHasErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
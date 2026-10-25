// Copyright 2026 kirzo

#include "KzGraphValidator.h"
#include "KzEdGraph.h"
#include "KzGraph.h"
#include "KzGraphCompiler.h"
#include "UObject/Package.h"

FName UKzGraphValidator::GetValidatorId_Implementation() const
{
	return KzGraphCompiler::ValidatorId;
}

bool UKzGraphValidator::CanValidate_Implementation(const UObject* Asset) const
{
	return Asset && Asset->IsA<UKzGraph>();
}

void UKzGraphValidator::Validate_Implementation(const UObject* Asset, TArray<FKzValidationIssue>& OutIssues) const
{
	const UKzEdGraph* EdGraph = Cast<UKzEdGraph>(CastChecked<UKzGraph>(Asset)->EdGraph);
	if (!EdGraph)
	{
		return;
	}

	FKzGraphData Data;
	KzGraphCompiler::Compile(*EdGraph, *GetTransientPackage(), Data, OutIssues);
}
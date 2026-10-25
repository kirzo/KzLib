// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"

class UKzEdGraph;
struct FKzGraphData;

namespace KzGraphCompiler
{
	/** Validator id of the problems a compile finds. */
	KZGRAPHEDITOR_API extern const FName ValidatorId;

	/** Compiles an editor graph into data whose instanced objects belong to Outer, gathering the problems found. */
	KZGRAPHEDITOR_API void Compile(const UKzEdGraph& Graph, UObject& Outer, FKzGraphData& OutData, TArray<FKzValidationIssue>& OutIssues);
}
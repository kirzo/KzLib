// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"

struct FKzDatabase;

namespace KzDatabaseValidation
{
	/**
	 * A table's issues: no data type (without ContextIndex), and by row (ContextIndex the row): repeated IDs, values of another type or empty,
	 * rows an earlier twin always beats, and rows without tags.
	 */
	KZLIBEDITOR_API void ValidateTable(const FKzDatabase& Database, FName ValidatorId, TArray<FKzValidationIssue>& OutIssues);
}
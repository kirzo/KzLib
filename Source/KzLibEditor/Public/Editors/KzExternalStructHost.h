// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KzExternalStructHost.generated.h"

/**
 * Trivial transient UObject used as the "anchor" for IDetailsView when injecting
 * external FStructOnScope instances via IDetailCategoryBuilder::AddExternalStructure.
 *
 * IDetailsView requires a UObject root to render anything, but we only want the
 * injected structs to show — so we use a class with no UPROPERTY of its own. The
 * class also serves as a stable key for RegisterInstancedCustomPropertyLayout
 * without polluting the global UObject namespace.
 */
UCLASS(Transient)
class KZLIBEDITOR_API UKzExternalStructHost : public UObject
{
	GENERATED_BODY()
};
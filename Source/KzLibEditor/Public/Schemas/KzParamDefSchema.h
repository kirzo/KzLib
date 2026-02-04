// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphSchema_K2.h"
#include "KzParamDefSchema.generated.h"

/**
 * Custom Schema used by FKzParamDefCustomization.
 * Filters out unsupported types (Interfaces, Maps, Sets) for the KzLib parameter system.
 */
UCLASS()
class KZLIBEDITOR_API UKzParamDefSchema : public UEdGraphSchema_K2
{
	GENERATED_BODY()

public:
	/** Generates the filtered list of types for the PinTypeSelector. */
	void GetKzParamTypeTree(TArray<TSharedPtr<FPinTypeTreeInfo>>& TypeTree, ETypeTreeFilter TypeTreeFilter = ETypeTreeFilter::None) const;
	virtual bool SupportsPinTypeContainer(TWeakPtr<const FEdGraphSchemaAction> SchemaAction, const FEdGraphPinType& PinType, const EPinContainerType& ContainerType) const override;
};
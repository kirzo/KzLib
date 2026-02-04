// Copyright 2026 kirzo

#include "Schemas/KzParamDefSchema.h"

void UKzParamDefSchema::GetKzParamTypeTree(TArray<TSharedPtr<FPinTypeTreeInfo>>& TypeTree, ETypeTreeFilter TypeTreeFilter) const
{
	GetVariableTypeTree(TypeTree, TypeTreeFilter);

	TypeTree.RemoveAll([](const TSharedPtr<FPinTypeTreeInfo>& Info)
		{
			// Info->GetPinType(false) returns the type representative of this tree node
			return Info->GetPinType(false).PinCategory == PC_Interface;
		});
}

bool UKzParamDefSchema::SupportsPinTypeContainer(TWeakPtr<const FEdGraphSchemaAction> SchemaAction, const FEdGraphPinType& PinType, const EPinContainerType& ContainerType) const
{
	// Only support Single values and Arrays. 
	return ContainerType == EPinContainerType::None || ContainerType == EPinContainerType::Array;
}
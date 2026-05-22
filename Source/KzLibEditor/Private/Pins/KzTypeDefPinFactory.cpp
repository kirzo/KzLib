// Copyright 2026 kirzo

#include "Pins/KzTypeDefPinFactory.h"
#include "Pins/SKzTypeDefGraphPin.h"
#include "Core/KzTypeDef.h"
#include "EdGraphSchema_K2.h"

TSharedPtr<SGraphPin> FKzTypeDefPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (!InPin)
	{
		return nullptr;
	}

	if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && InPin->PinType.PinSubCategoryObject == FKzTypeDef::StaticStruct())
	{
		return SNew(SKzTypeDefGraphPin, InPin);
	}

	return nullptr;
}
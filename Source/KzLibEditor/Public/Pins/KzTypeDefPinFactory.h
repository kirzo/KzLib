// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Pin factory that swaps the default struct pin widget for an inline type picker whenever an FKzTypeDef pin is shown on a graph node.
 */
class KZLIBEDITOR_API FKzTypeDefPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(UEdGraphPin* InPin) const override;
};
// Copyright 2026 kirzo

#pragma once

#include "K2Node_CallFunction.h"
#include "K2Node_KzVariant.generated.h"

class UEdGraphPin;

/** Node customization for UKzVariantLibrary */
UCLASS()
class UK2Node_KzVariant : public UK2Node_CallFunction
{
	GENERATED_BODY()

	//~ Begin UEdGraphNode Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	//~ End K2Node Interface
};
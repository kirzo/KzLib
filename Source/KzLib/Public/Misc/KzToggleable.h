// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KzToggleable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UKzToggleable : public UInterface
{
	GENERATED_BODY()
};

/** Something that can be switched on and off as a whole, such as a limit or a door, by whoever only needs it at times. */
class KZLIB_API IKzToggleable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KzLib")
	void SetEnabled(bool bNewEnabled);
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KzInputModifier.generated.h"

/**
 * Base class for any object that modifies an analog input vector (Movement, Look, etc.).
 * Designed to be stacked.
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, CollapseCategories)
class KZLIB_API UKzInputModifier : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the World context (helper for Blueprint modifiers). */
	virtual UWorld* GetWorld() const override
	{
		if (IsTemplate()) return nullptr;
		return GetOuter() ? GetOuter()->GetWorld() : nullptr;
	}

	/**
	 * Calculates the modified input.
	 * @param OriginalInput The raw input from the controller (before any modifier in the stack).
	 * @param CurrentInput The input as modified by previous modifiers in the stack.
	 * @return The new input vector.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Input Modifier")
	FVector ModifyInput(const FVector& OriginalInput, const FVector& CurrentInput);

private:
	virtual FVector ModifyInput_Implementation(const FVector& OriginalInput, const FVector& CurrentInput)
	{
		return CurrentInput;
	}
};
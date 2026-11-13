// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Containers/KzShuffleBag.h"
#include "KzShuffleBagLibrary.generated.h"

/** Shuffle bags from Blueprint: values come in and out through pins of the bag's type. */
UCLASS()
class KZLIB_API UKzShuffleBagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The next value, into a pin of the bag's type. False when no entry has weight or the pin is of another type. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag", CustomThunk, meta = (CustomStructureParam = "Value"))
	static bool DrawFromShuffleBag(UPARAM(ref) FKzShuffleBag& Bag, int32& Value);

	/** The next entry's index, INDEX_NONE when none has weight. The way to draw from a bag of weights alone. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag")
	static int32 DrawIndexFromShuffleBag(UPARAM(ref) FKzShuffleBag& Bag);

	/** Adds a value of the bag's type; a bag without a type nor entries takes the value's. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag", CustomThunk, meta = (CustomStructureParam = "Value"))
	static bool AddToShuffleBag(UPARAM(ref) FKzShuffleBag& Bag, const int32& Value, int32 Weight = 1);

	/** Adds an entry without a value, to a bag without a type. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag")
	static bool AddWeightToShuffleBag(UPARAM(ref) FKzShuffleBag& Bag, int32 Weight = 1);

	/** How many times an entry comes out a round. A change deals a new round. False without that entry. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag")
	static bool SetShuffleBagWeight(UPARAM(ref) FKzShuffleBag& Bag, int32 Index, int32 Weight);

	/** The next draw deals a new round. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag")
	static void ResetShuffleBag(UPARAM(ref) FKzShuffleBag& Bag);

	UFUNCTION(BlueprintCallable, Category = "KzLib|Shuffle Bag")
	static void EmptyShuffleBag(UPARAM(ref) FKzShuffleBag& Bag);

private:
	DECLARE_FUNCTION(execDrawFromShuffleBag);
	DECLARE_FUNCTION(execAddToShuffleBag);
};
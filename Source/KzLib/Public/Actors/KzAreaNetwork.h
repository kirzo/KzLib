// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/KzComponentReference.h"
#include "KzAreaNetwork.generated.h"

UENUM(BlueprintType)
enum class EKzAreaOperation : uint8
{
	Add, Subtract
};

USTRUCT(BlueprintType)
struct KZLIB_API FKzAreaModifier
{
	GENERATED_BODY()

	/** How this area affects the final network. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Modifier")
	EKzAreaOperation Operation = EKzAreaOperation::Subtract;

	/** Reference to the Spline Area Component in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Modifier", meta = (AllowedClasses = "/Script/KzLib.KzSplineAreaComponent"))
	FKzComponentReference AreaReference;
};

UCLASS(HideCategories = (Rendering, Tags, Activation, Collision, Cooking, Replication, Input, Actor, Physics, LOD, AssetUserData), ClassGroup = (KzLib), meta = (DisplayName = "Spline Area Network"))
class KZLIB_API AKzAreaNetwork : public AActor
{
	GENERATED_BODY()

public:
	AKzAreaNetwork(const FObjectInitializer& ObjectInitializer);

	/** The base additive area of this network. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Area Network")
	TObjectPtr<class UKzSplineAreaComponent> BaseArea;

	/** Additional areas to merge or subtract from the base area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area Network")
	TArray<FKzAreaModifier> AreaModifiers;

	/** Checks if a point is inside the logical sum/subtraction of the network. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Area Network")
	bool IsPointInside(const FVector& Point) const;

	/** Generates a valid random point respecting all additions and subtractions. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Area Network")
	FVector GetRandomPointInside(int32 MaxAttempts = 100) const;
};
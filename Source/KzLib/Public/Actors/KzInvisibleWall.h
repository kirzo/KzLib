// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/KzToggleable.h"
#include "KzInvisibleWall.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UBoxComponent;

/**
 * Invisible box that blocks pawns, such as the edge of an arena. It doesn't cut the navmesh, so AI paths through it.
 * One Way lets each pawn through from behind its arrow and blocks it once it is through.
 */
UCLASS(Blueprintable, HideCategories = (Input, Physics, LOD, Cooking, Replication, HLOD))
class KZLIB_API AKzInvisibleWall : public AActor, public IKzToggleable
{
	GENERATED_BODY()

public:
	AKzInvisibleWall();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Invisible Wall")
	bool bEnabled = true;

	/** Pawns only cross it the way its arrow points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Invisible Wall")
	bool bOneWay = false;

	virtual void SetEnabled_Implementation(bool bNewEnabled) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Invisible Wall")
	TObjectPtr<UBoxComponent> WallComponent;

	/** Finds the pawns near a One Way wall. */
	UPROPERTY()
	TObjectPtr<UBoxComponent> NearbyComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TObjectPtr<UArrowComponent> DirectionArrow;
#endif

private:
	/** Pawns near a One Way wall, by the component they move, and whether the wall lets each through. */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> Nearby;

	void Refresh();
	void ReleaseNearby();

	/** Starts watching a pawn's moving component near the wall. */
	void Watch(UPrimitiveComponent* Component);

	/** Behind the wall, or crossing it from behind and not through yet. */
	bool IsCrossing(const UPrimitiveComponent& Mover, bool bWasCrossing) const;

	UFUNCTION()
	void HandleNearbyBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleNearbyEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
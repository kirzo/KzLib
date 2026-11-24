// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Interface.h"
#include "UObject/ObjectKey.h"
#include "KzActorPoolSubsystem.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UKzPoolable : public UInterface
{
	GENERATED_BODY()
};

/** An actor, or a component of one, that resets what the pool doesn't when its actor goes back to the pool or comes out again. */
class KZLIB_API IKzPoolable
{
	GENERATED_BODY()

public:
	/** The pool handed the actor out again, already in its new place. A new actor gets BeginPlay instead. */
	UFUNCTION(BlueprintNativeEvent, Category = "KzLib|Pool")
	void OnAcquiredFromPool();

	/** The actor went back to the pool: undo what its last life left, so it comes out as a new one would. */
	UFUNCTION(BlueprintNativeEvent, Category = "KzLib|Pool")
	void OnReleasedToPool();
};

/**
 * Keeps actors that are done, hidden, to hand them out again instead of spawning new ones, such as enemies or projectiles.
 *
 * Back in the pool an actor is hidden, without collision or tick, and stops moving; a pawn loses its controller as it would on being destroyed.
 * Handed out it gets them back, and a pawn that makes its own AI controller makes a new one. Its construction script and BeginPlay don't run
 * again: whatever else its last life left is for IKzPoolable, on the actor and on its components.
 */
UCLASS()
class KZLIB_API UKzActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UKzActorPoolSubsystem* Get(const UObject* WorldContextObject);

	/** An actor of Class at Transform: one from the pool if it has any, or a new one. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Pool", meta = (DeterminesOutputType = "Class"))
	AActor* Acquire(TSubclassOf<AActor> Class, const FTransform& Transform);

	/** Takes the actor instead of destroying it, for Acquire to hand it out again. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Pool")
	void Release(AActor* Actor);

	/** Spawns actors of Class into the pool until it holds Count, so Acquire spawns none later, such as while the level loads. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Pool")
	void Prewarm(TSubclassOf<AActor> Class, int32 Count);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** An actor in the pool, with what the pool turned off and gives back. */
	struct FPooledActor
	{
		TWeakObjectPtr<AActor> Actor;
		bool bHidden = false;
		bool bCollision = true;
		bool bTick = true;
		TArray<TWeakObjectPtr<UActorComponent>> TickingComponents;
	};

	TMap<TObjectKey<UClass>, TArray<FPooledActor>> Pools;
};
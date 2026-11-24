// Copyright 2026 kirzo

#include "Pooling/KzActorPoolSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Pawn.h"

namespace KzActorPool
{
	template <typename FuncType>
	void ForEachPoolable(AActor& Actor, FuncType&& Func)
	{
		if (Actor.Implements<UKzPoolable>())
		{
			Func(&Actor);
		}

		TInlineComponentArray<UActorComponent*> Components(&Actor);
		for (UActorComponent* Component : Components)
		{
			if (Component->Implements<UKzPoolable>())
			{
				Func(Component);
			}
		}
	}
}

UKzActorPoolSubsystem* UKzActorPoolSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UKzActorPoolSubsystem>() : nullptr;
}

bool UKzActorPoolSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

AActor* UKzActorPoolSubsystem::Acquire(TSubclassOf<AActor> Class, const FTransform& Transform)
{
	if (!Class)
	{
		return nullptr;
	}

	if (TArray<FPooledActor>* Pool = Pools.Find(TObjectKey<UClass>(Class.Get())))
	{
		while (!Pool->IsEmpty())
		{
			const FPooledActor Entry = Pool->Pop(EAllowShrinking::No);
			AActor* Actor = Entry.Actor.Get();
			if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
			{
				continue;
			}

			Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);
			Actor->SetActorHiddenInGame(Entry.bHidden);
			Actor->SetActorEnableCollision(Entry.bCollision);
			Actor->SetActorTickEnabled(Entry.bTick);
			for (const TWeakObjectPtr<UActorComponent>& Component : Entry.TickingComponents)
			{
				if (Component.IsValid())
				{
					Component->SetComponentTickEnabled(true);
				}
			}

			KzActorPool::ForEachPoolable(*Actor, [](UObject* Object) { IKzPoolable::Execute_OnAcquiredFromPool(Object); });

			// After the reset, so what possession starts finds the actor as new
			APawn* Pawn = Cast<APawn>(Actor);
			if (Pawn && !Pawn->GetController() && (Pawn->AutoPossessAI == EAutoPossessAI::Spawned || Pawn->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned))
			{
				Pawn->SpawnDefaultController();
			}

			return Actor;
		}
	}

	return GetWorld()->SpawnActor<AActor>(Class, Transform);
}

void UKzActorPoolSubsystem::Release(AActor* Actor)
{
	if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
	{
		return;
	}

	const TObjectKey<UClass> ClassKey(Actor->GetClass());
	const auto IsActor = [Actor](const FPooledActor& Entry) { return Entry.Actor == Actor; };

	if (const TArray<FPooledActor>* Pool = Pools.Find(ClassKey); Pool && Pool->ContainsByPredicate(IsActor))
	{
		return;
	}

	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		Pawn->DetachFromControllerPendingDestroy();
	}

	KzActorPool::ForEachPoolable(*Actor, [](UObject* Object) { IKzPoolable::Execute_OnReleasedToPool(Object); });

	// Found again: a reset may have released other actors
	FPooledActor& Entry = Pools.FindOrAdd(ClassKey).AddDefaulted_GetRef();
	Entry.Actor = Actor;
	Entry.bHidden = Actor->IsHidden();
	Entry.bCollision = Actor->GetActorEnableCollision();
	Entry.bTick = Actor->IsActorTickEnabled();

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Component : Components)
	{
		if (UMovementComponent* Movement = Cast<UMovementComponent>(Component))
		{
			Movement->StopMovementImmediately();
		}

		if (Component->IsComponentTickEnabled())
		{
			Component->SetComponentTickEnabled(false);
			Entry.TickingComponents.Add(Component);
		}
	}
}

void UKzActorPoolSubsystem::Prewarm(TSubclassOf<AActor> Class, int32 Count)
{
	if (!Class)
	{
		return;
	}

	const TArray<FPooledActor>* Pool = Pools.Find(TObjectKey<UClass>(Class.Get()));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Missing = Count - (Pool ? Pool->Num() : 0); Missing > 0; --Missing)
	{
		Release(GetWorld()->SpawnActor<AActor>(Class, FTransform::Identity, Params));
	}
}
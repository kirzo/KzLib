// Copyright 2026 kirzo

#include "Components/KzComponentSocketReference.h"
#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

USceneComponent* FKzComponentSocketReference::FindComponentInActor(const AActor* InActor, FName InName) const
{
	if (!InActor) return nullptr;

	FObjectPropertyBase* ObjProp = FindFProperty<FObjectPropertyBase>(InActor->GetClass(), InName);
	if (ObjProp != nullptr)
	{
		return Cast<USceneComponent>(ObjProp->GetObjectPropertyValue_InContainer(InActor));
	}

	return nullptr;
}

USceneComponent* FKzComponentSocketReference::GetComponent(const UObject* ContextObject) const
{
	// Determine priority: OverrideActor takes precedence.
	if (OverrideActor)
	{
		// If explicit Actor is set, we behave relative to THAT actor.
		if (ComponentName.IsNone())
		{
			return OverrideActor->GetRootComponent();
		}

		// If name specified, find in that actor
		return FindComponentInActor(OverrideActor.Get(), ComponentName);
	}

	// No Override
	if (!ContextObject)
	{
		return nullptr;
	}

	// We MUST find the owning Actor to search for the property/component name.
	if (!ComponentName.IsNone())
	{
		const AActor* OwnerActor = nullptr;

		// Traverse up to find the Actor
		const UObject* Current = ContextObject;
		while (Current)
		{
			if (const AActor* Act = Cast<AActor>(Current))
			{
				OwnerActor = Act;
				break;
			}
			Current = Current->GetOuter();
		}

		if (OwnerActor)
		{
			return FindComponentInActor(OwnerActor, ComponentName);
		}

		return nullptr; // Name specified but no Actor found in hierarchy
	}

	// Traverse up to find the closest valid SceneComponent or Actor Root.
	const UObject* Current = ContextObject;
	while (Current)
	{
		// If we are (or contain inside) a SceneComponent, return it directly.
		if (const USceneComponent* ScnComp = Cast<USceneComponent>(Current))
		{
			return const_cast<USceneComponent*>(ScnComp);
		}

		// If we hit the Actor, fallback to its RootComponent.
		if (const AActor* Act = Cast<AActor>(Current))
		{
			return Act->GetRootComponent();
		}

		// Move up the chain (e.g. UObject -> Component -> Actor)
		Current = Current->GetOuter();
	}

	return nullptr;
}

bool FKzComponentSocketReference::GetSocketTransform(const UObject* ContextObject, FTransform& OutTransform) const
{
	USceneComponent* TargetComp = GetComponent(ContextObject);
	if (!TargetComp)
	{
		return false;
	}

	OutTransform = GetRelativeTransform() * TargetComp->GetSocketTransform(SocketName);
	return true;
}

FKzTransformSource FKzComponentSocketReference::ToTransformSource(const UObject* ContextObject) const
{
	USceneComponent* TargetComp = GetComponent(ContextObject);
	return FKzTransformSource(TargetComp, SocketName, GetRelativeTransform());
}
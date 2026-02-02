// Copyright 2026 kirzo

#include "Components/KzComponentSocketReference.h"
#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

USceneComponent* FKzComponentSocketReference::GetComponent(const AActor* OwnerActor) const
{
	// Determine Target Actor (Priority: OverrideActor > OwnerActor)
	const AActor* TargetActor = OverrideActor ? OverrideActor.Get() : OwnerActor;

	if (!TargetActor)
	{
		return nullptr;
	}

	if (!ComponentName.IsNone())
	{
		FObjectPropertyBase* ObjProp = FindFProperty<FObjectPropertyBase>(TargetActor->GetClass(), ComponentName);
		if (ObjProp != NULL)
		{
			return Cast<USceneComponent>(ObjProp->GetObjectPropertyValue_InContainer(TargetActor));
		}
	}
	else
	{
		return TargetActor->GetRootComponent();
	}

	return nullptr;
}

bool FKzComponentSocketReference::GetSocketTransform(const AActor* OwnerActor, FTransform& OutTransform) const
{
	USceneComponent* TargetComp = GetComponent(OwnerActor);
	if (!TargetComp)
	{
		return false;
	}

	OutTransform = GetRelativeTransform() * TargetComp->GetSocketTransform(SocketName);
	return true;
}

FKzTransformSource FKzComponentSocketReference::ToTransformSource(const AActor* OwnerActor) const
{
	USceneComponent* TargetComp = GetComponent(OwnerActor);
	return FKzTransformSource(TargetComp, SocketName, GetRelativeTransform());
}
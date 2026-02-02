// Copyright 2026 kirzo

#include "Components/KzComponentSocketReference.h"
#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

USceneComponent* FKzComponentSocketReference::GetComponent(AActor* OwnerActor) const
{
	// Determine Target Actor (Priority: OverrideActor > OwnerActor)
	AActor* TargetActor = OverrideActor ? OverrideActor.Get() : OwnerActor;

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

bool FKzComponentSocketReference::GetSocketTransform(AActor* OwnerActor, FTransform& OutTransform) const
{
	USceneComponent* TargetComp = GetComponent(OwnerActor);
	if (!TargetComp)
	{
		return false;
	}

	OutTransform = RelativeTransform * TargetComp->GetSocketTransform(SocketName);
	return true;
}
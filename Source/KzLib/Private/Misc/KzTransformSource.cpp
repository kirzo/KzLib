// Copyright 2026 kirzo

#include "Misc/KzTransformSource.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

FKzTransformSource::FKzTransformSource(const AActor* Actor, FVector RelativeLocation)
	: FKzTransformSource(Actor, FTransform(RelativeLocation))
{
}

FKzTransformSource::FKzTransformSource(const AActor* Actor, FTransform RelativeTransform)
{
	Initialize(Actor, RelativeTransform);
}

FKzTransformSource::FKzTransformSource(const USceneComponent* SceneComponent, FName SocketName, FVector RelativeLocation)
	: FKzTransformSource(SceneComponent, SocketName, FTransform(RelativeLocation))
{
}

FKzTransformSource::FKzTransformSource(const USceneComponent* SceneComponent, FName SocketName, FTransform RelativeTransform)
{
	Initialize(SceneComponent, SocketName, RelativeTransform);
}

FKzTransformSource::FKzTransformSource(FVector Location)
	: FKzTransformSource(FTransform(Location))
{
}

FKzTransformSource::FKzTransformSource(FQuat Quat)
	: FKzTransformSource(FTransform(Quat))
{
}

FKzTransformSource::FKzTransformSource(FRotator Rotation)
	: FKzTransformSource(FTransform(Rotation))
{
}

FKzTransformSource::FKzTransformSource(FTransform Transform)
{
	Initialize(Transform);
}

void FKzTransformSource::Initialize(const AActor* Actor, FVector RelativeLocation)
{
	Initialize(Actor, FTransform(RelativeLocation));
}

void FKzTransformSource::Initialize(const AActor* Actor, FTransform RelativeTransform)
{
	Reset();
	if (Actor)
	{
		SourceType = EKzTransformSourceType::Actor;
		SourceActor = Actor;
		LiteralTransform = RelativeTransform;
	}
}

void FKzTransformSource::Initialize(const USceneComponent* SceneComponent, FName SocketName, FVector RelativeLocation)
{
	Initialize(SceneComponent, SocketName, FTransform(RelativeLocation));
}

void FKzTransformSource::Initialize(const USceneComponent* SceneComponent, FName SocketName, FTransform RelativeTransform)
{
	Reset();
	if (SceneComponent)
	{
		SourceType = EKzTransformSourceType::Scene;
		SourceComponent = SceneComponent;
		SourceSocketName = SocketName;
		LiteralTransform = RelativeTransform;
	}
}

void FKzTransformSource::Initialize(FVector Location)
{
	Initialize(FTransform(Location));
}

void FKzTransformSource::Initialize(FQuat Quat)
{
	Initialize(FTransform(Quat));
}

void FKzTransformSource::Initialize(FRotator Rotation)
{
	Initialize(FTransform(Rotation));
}

void FKzTransformSource::Initialize(FTransform Transform)
{
	Reset();
	SourceType = EKzTransformSourceType::Literal;
	LiteralTransform = Transform;
}

FKzTransformSource FKzTransformSource::MakeFromComponentRef(AActor* OwningActor, const FComponentReference& ComponentRef, FName SocketName, FVector RelativeLocation)
{
	return MakeFromComponentRef(OwningActor, ComponentRef, SocketName, FTransform(RelativeLocation));
}

FKzTransformSource FKzTransformSource::MakeFromComponentRef(AActor* OwningActor, const FComponentReference& ComponentRef, FName SocketName, FTransform RelativeTransform)
{
	FKzTransformSource This(NoInit);
	if (USceneComponent* Component = Cast<USceneComponent>(ComponentRef.GetComponent(OwningActor)))
	{
		This.Initialize(Component, SocketName, RelativeTransform);
	}
	return This;
}

void FKzTransformSource::Reset()
{
	LiteralTransform = FTransform::Identity;
	SourceSocketName = NAME_None;
	Clear();
}

void FKzTransformSource::Clear()
{
	SourceType = EKzTransformSourceType::Invalid;
	SourceActor = nullptr;
	SourceComponent = nullptr;
}

FVector FKzTransformSource::GetLocation() const
{
	return GetTransform().GetLocation();
}

FQuat FKzTransformSource::GetQuat() const
{
	return GetTransform().GetRotation();
}

FRotator FKzTransformSource::GetRotation() const
{
	return GetTransform().Rotator();
}

FTransform FKzTransformSource::GetTransform() const
{
	// Return or calculate based on SourceType.
	switch (SourceType)
	{
		case EKzTransformSourceType::Actor:
		{
			if (SourceActor)
			{
				return LiteralTransform * SourceActor->GetTransform();
			}
			break;
		}
		case EKzTransformSourceType::Scene:
		{
			if (SourceComponent)
			{
				// Bad socket name will just return component transform anyway, so we're safe
				return LiteralTransform * SourceComponent->GetSocketTransform(SourceSocketName);
			}
			break;
		}
		case EKzTransformSourceType::Literal:
		{
			return LiteralTransform;
		}
		case EKzTransformSourceType::Invalid:
		{
			return FTransform::Identity;
		}
		default:
		{
			check(false);
			break;
		}
	}

	// It cannot get here
	return FTransform::Identity;
}

FVector FKzTransformSource::GetRelativeLocation() const
{
	return GetRelativeTransform().GetLocation();
}

FQuat FKzTransformSource::GetRelativeQuat() const
{
	return GetRelativeTransform().GetRotation();
}

FRotator FKzTransformSource::GetRelativeRotation() const
{
	return GetRelativeTransform().Rotator();
}

FTransform FKzTransformSource::GetRelativeTransform() const
{
	// Return or calculate based on SourceType.
	switch (SourceType)
	{
		case EKzTransformSourceType::Actor:
		case EKzTransformSourceType::Scene:
		case EKzTransformSourceType::Literal:
		{
			return LiteralTransform;
		}
		case EKzTransformSourceType::Invalid:
		{
			return FTransform::Identity;
		}
		default:
		{
			check(false);
			break;
		}
	}

	// It cannot get here
	return FTransform::Identity;
}

const AActor* FKzTransformSource::GetActor() const
{
	switch (SourceType)
	{
		case EKzTransformSourceType::Actor:
		{
			return SourceActor;
		}
		case EKzTransformSourceType::Scene:
		{
			return SourceComponent ? SourceComponent->GetOwner() : nullptr;
		}
	}

	return nullptr;
}

const USceneComponent* FKzTransformSource::GetSceneComponent() const
{
	switch (SourceType)
	{
		case EKzTransformSourceType::Actor:
		{
			return SourceActor ? SourceActor->GetRootComponent() : nullptr;
		}
		case EKzTransformSourceType::Scene:
		{
			return SourceComponent;
		}
	}

	return nullptr;
}

void FKzTransformSource::Invalidate()
{
	*this = FKzTransformSource();
}

bool FKzTransformSource::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << (uint8&)SourceType;

	switch (SourceType)
	{
		case EKzTransformSourceType::Invalid:
		{
			break;
		}
		case EKzTransformSourceType::Actor:
		{
			Ar << LiteralTransform;
			Ar << SourceActor;
			break;
		}
		case EKzTransformSourceType::Scene:
		{
			Ar << LiteralTransform;
			Ar << SourceComponent;
			Ar << SourceSocketName;
			break;
		}
		case EKzTransformSourceType::Literal:
		{
			Ar << LiteralTransform;
			break;
		}
		default:
		check(false);	//This case should not happen
		break;
	}

	bOutSuccess = true;
	return true;
}

bool FKzTransformSource::operator==(const FKzTransformSource& Other) const
{
	if (Other.SourceType == SourceType)
	{
		switch (SourceType)
		{
			case EKzTransformSourceType::Actor:
			{
				return SourceActor == Other.SourceActor && LiteralTransform.Equals(Other.LiteralTransform);
			}
			case EKzTransformSourceType::Scene:
			{
				return SourceComponent == Other.SourceComponent && SourceSocketName == Other.SourceSocketName && LiteralTransform.Equals(Other.LiteralTransform);
			}
			case EKzTransformSourceType::Literal:
			{
				return LiteralTransform.Equals(Other.LiteralTransform);
			}
			case EKzTransformSourceType::Invalid:
			{
				return true;
			}
			default:
			{
				check(false);
				break;
			}
		}
	}
	return false;
}

bool FKzTransformSource::operator!=(const FKzTransformSource& Other) const
{
	return !(*this == Other);
}
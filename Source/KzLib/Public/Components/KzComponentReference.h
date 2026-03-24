// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Misc/KzTransformSource.h"
#include "KzComponentReference.generated.h"

class AActor;
class USceneComponent;

/**
 * A pure reference to a specific Component within an Actor.
 * Supports context resolution for Instanced Objects, Child Actor Components, and Actor overrides.
 *
 * Supported Metadata (UPROPERTY meta tags):
 *
 * - AllowedClasses:
 * Filters the Component dropdown to show only specific types.
 * Supports simple class names or full paths.
 * Example: meta = (AllowedClasses = "/Script/Engine.SkeletalMeshComponent, /Script/Engine.SplineComponent")
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzComponentReference
{
	GENERATED_BODY()

public:
	FKzComponentReference() = default;
	FKzComponentReference(FName InComponentName) : ComponentName(InComponentName) {}

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Reference")
	TObjectPtr<AActor> OverrideActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reference")
	FName ComponentName;

	USceneComponent* GetComponent(const UObject* ContextObject) const;

	template <typename T>
	T* GetComponent(const UObject* ContextObject) const
	{
		static_assert(TIsDerivedFrom<T, USceneComponent>::IsDerived, "T must be derived from USceneComponent");
		return Cast<T>(GetComponent(ContextObject));
	}

	/** Implicit conversion to FName */
	FORCEINLINE operator FName() const { return ComponentName; }

	bool operator==(const FKzComponentReference& Other) const
	{
		return OverrideActor == Other.OverrideActor && ComponentName == Other.ComponentName;
	}

	friend uint32 GetTypeHash(const FKzComponentReference& Ref)
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(Ref.OverrideActor.Get()));
		Hash = HashCombine(Hash, GetTypeHash(Ref.ComponentName));
		return Hash;
	}

private:
	USceneComponent* FindComponentInActor(const AActor* InActor, FName InName) const;
};

/**
 * A reference to a specific Socket on a specific Component.
 * Supports context resolution for Instanced Objects, Child Actor Components, Actor overrides, and relative local offsets.
 *
 * Supported Metadata (UPROPERTY meta tags):
 *
 * - NoSocket:
 * If set to "true", hides the Socket selector widget.
 * Useful when you need to reference a Component object itself (e.g., for logic or property access)
 * rather than a specific transform point within it.
 *
 * - NoOffset:
 * If set to "true", hides the RelativeLocation and RelativeRotation properties in the Details panel.
 * Useful for strict attachments where manual offset is not allowed.
 *
 * - AllowedClasses:
 * Filters the Component dropdown to show only specific types.
 * Supports simple class names or full paths.
 * Example: meta = (AllowedClasses = "/Script/Engine.SkeletalMeshComponent, /Script/Engine.SplineComponent")
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzComponentSocketReference : public FKzComponentReference
{
	GENERATED_BODY()

public:
	/** Name of the Socket on that Component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reference")
	FName SocketName;

	/** Local position offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reference")
	FVector RelativeLocation = FVector::ZeroVector;

	/** Local rotation offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reference")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	FTransform GetRelativeTransform() const
	{
		return FTransform(RelativeRotation, RelativeLocation, FVector::OneVector);
	}

	/**
	 * Helper to resolve the reference and get the World Transform.
	 * @param ContextObject The object initiating the query. Can be an Actor, a Component, or any UObject.
	 * @param OutTransform The resulting transform.
	 * @return True if the component and socket (or component transform if socket is None) were found.
	 */
	bool GetSocketTransform(const UObject* ContextObject, FTransform& OutTransform) const;

	/**
	 * Converts this safe reference into a runtime Transform Source.
	 * @param ContextObject The object initiating the query. Can be an Actor, a Component, or any UObject.
	 */
	FKzTransformSource ToTransformSource(const UObject* ContextObject) const;

	bool operator==(const FKzComponentSocketReference& Other) const
	{
		return OverrideActor == Other.OverrideActor &&
			ComponentName == Other.ComponentName &&
			SocketName == Other.SocketName;
	}

	friend uint32 GetTypeHash(const FKzComponentSocketReference& Ref)
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(Ref.OverrideActor));
		Hash = HashCombine(Hash, GetTypeHash(Ref.ComponentName));
		Hash = HashCombine(Hash, GetTypeHash(Ref.SocketName));
		return Hash;
	}
};
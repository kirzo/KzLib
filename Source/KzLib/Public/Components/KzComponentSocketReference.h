// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Misc/KzTransformSource.h"
#include "KzComponentSocketReference.generated.h"

class AActor;
class USceneComponent;

/** A reference to a specific Socket on a specific Component. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzComponentSocketReference
{
	GENERATED_BODY()

public:
	/** If set, the component will be searched in this Actor instead of the owner. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Reference")
	TObjectPtr<AActor> OverrideActor = nullptr;

	/** Name of the Component to look for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reference")
	FName ComponentName;

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

	/** Resolves and returns the pointer to the scene component */
	USceneComponent* GetComponent(const AActor* OwnerActor) const;

	/**
	 * Helper to resolve the reference and get the World Transform.
	 * @param OwnerActor The actor that owns this structure (usually context from the caller).
	 * @param OutTransform The resulting transform.
	 * @return True if the component and socket (or component transform if socket is None) were found.
	 */
	bool GetSocketTransform(const AActor* OwnerActor, FTransform& OutTransform) const;

	/**
	 * Converts this safe reference into a runtime Transform Source.
	 * @param OwnerActor The context actor to resolve the component (if OverrideActor is null).
	 */
	FKzTransformSource ToTransformSource(const AActor* OwnerActor) const;
};
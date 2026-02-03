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

	/**
	 * Resolves the component.
	 * @param ContextObject The object initiating the query. Can be an Actor, a Component, or any UObject.
	 */
	USceneComponent* GetComponent(const UObject* ContextObject) const;

	/**
	 * Templated version of GetComponent to retrieve a specific component class.
	 * Resolves the component first, then attempts to cast it to T.
	 *
	 * @param ContextObject The object initiating the query.
	 * @return A pointer to the component cast to type T, or nullptr if the resolved component is not of type T.
	 */
	template <typename T>
	T* GetComponent(const UObject* ContextObject) const
	{
		// Ensure T is actually a SceneComponent at compile time.
		static_assert(TIsDerivedFrom<T, USceneComponent>::IsDerived, "T must be derived from USceneComponent");

		return Cast<T>(GetComponent(ContextObject));
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

private:
	USceneComponent* FindComponentInActor(const AActor* InActor, FName InName) const;
};
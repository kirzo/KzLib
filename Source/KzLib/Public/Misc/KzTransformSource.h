// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "KzTransformSource.generated.h"

/** Defines the different ways a transform source can provide its world-space transform. */
UENUM(BlueprintType)
enum class EKzTransformSourceType : uint8
{
	/** Transform is not set or invalid. */
	Invalid UMETA(DisplayName = "None"),

	/** The transform is provided as a literal world-space value. */
	Literal  UMETA(DisplayName = "Literal Transform"),

	/** The transform is derived directly from an actor’s world transform. */
	Actor  UMETA(DisplayName = "Actor"),

	/** The transform is derived from a scene component or one of its named sockets. */
	Scene  UMETA(DisplayName = "Scene Component")
};

/** Represents a flexible spatial reference that can resolve to an Actor, Component, Socket, or literal Transform. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzTransformSource
{
	GENERATED_BODY()

	/** The type of source used to resolve the transform. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = KzTransformSource)
	EKzTransformSourceType SourceType = EKzTransformSourceType::Invalid;

	/**
	 * A literal transform in world space. Used when SourceType == Literal.
	 * Applied as a local-space offset relative to the source, if any.
	 */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = KzTransformSource)
	FTransform LiteralTransform;

	/** The actor used when SourceType == Actor. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = KzTransformSource)
	TObjectPtr<const AActor> SourceActor;

	/** The scene component used when SourceType == Scene. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = KzTransformSource)
	TObjectPtr<const USceneComponent> SourceComponent;

	/** The socket name used when SourceType == Scene and the component supports sockets. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = KzTransformSource)
	FName SourceSocketName;

	FKzTransformSource() = default;
	FKzTransformSource(ENoInit) {}
	FKzTransformSource(EForceInit) { SourceType = EKzTransformSourceType::Literal; }
	FKzTransformSource(const AActor* Actor, FVector RelativeLocation = FVector::ZeroVector);
	FKzTransformSource(const AActor* Actor, FTransform RelativeTransform = FTransform::Identity);
	FKzTransformSource(const USceneComponent* SceneComponent, FName SocketName = NAME_None, FVector RelativeLocation = FVector::ZeroVector);
	FKzTransformSource(const USceneComponent* SceneComponent, FName SocketName = NAME_None, FTransform RelativeTransform = FTransform::Identity);
	FKzTransformSource(FVector Location);
	FKzTransformSource(FQuat Quat);
	FKzTransformSource(FRotator Rotation);
	FKzTransformSource(FTransform Transform);

	void Initialize(const AActor* Actor, FVector RelativeLocation = FVector::ZeroVector);
	void Initialize(const AActor* Actor, FTransform RelativeTransform = FTransform::Identity);
	void Initialize(const USceneComponent* SceneComponent, FName SocketName = NAME_None, FVector RelativeLocation = FVector::ZeroVector);
	void Initialize(const USceneComponent* SceneComponent, FName SocketName = NAME_None, FTransform RelativeTransform = FTransform::Identity);
	void Initialize(FVector Location);
	void Initialize(FQuat Quat);
	void Initialize(FRotator Rotation);
	void Initialize(FTransform Transform);

	/**
	 * Creates a transform source from a component reference within the given actor.
	 *
	 * If the component reference resolves successfully, the source will be initialized
	 * from that component using the provided socket name and relative transform.
	 * Otherwise, the actor’s root component will be used as a fallback.
	 *
	 * @param OwningActor       The actor that owns the referenced component.
	 * @param ComponentRef      Reference to the component within the actor.
	 * @param SocketName        Optional socket name on the component.
	 * @param RelativeLocation  Local-space location offset relative to the resolved component or socket.
	 * @return A fully initialized transform source, or an invalid one if no valid actor was provided.
	 */
	static FKzTransformSource MakeFromComponentRef(AActor* OwningActor, const FComponentReference& ComponentRef, FName SocketName = NAME_None, FVector RelativeLocation = FVector::ZeroVector);

	/**
	 * Creates a transform source from a component reference within the given actor.
	 *
	 * If the component reference resolves successfully, the source will be initialized
	 * from that component using the provided socket name and relative transform.
	 * Otherwise, the actor’s root component will be used as a fallback.
	 *
	 * @param OwningActor       The actor that owns the referenced component.
	 * @param ComponentRef      Reference to the component within the actor.
	 * @param SocketName        Optional socket name on the component.
	 * @param RelativeTransform Local-space transform offset relative to the resolved component or socket.
	 * @return A fully initialized transform source, or an invalid one if no valid actor was provided.
	 */
	static FKzTransformSource MakeFromComponentRef(AActor* OwningActor, const FComponentReference& ComponentRef, FName SocketName = NAME_None, FTransform RelativeTransform = FTransform::Identity);

	/**
	 * Resets the transform source to a default, identity state.
	 * Clears all actor, component, and socket references,
	 * and restores the literal transform to identity.
	 *
	 * Use this when you want to fully reset the source.
	 */
	void Reset();

	/**
	 * Clears all internal references and marks the source as invalid.
	 * Unlike Reset(), this does not restore the literal transform — it simply invalidates the source.
	 *
	 * Use this to fully invalidate the source.
	 */
	void Clear();

	template<typename T>
	inline T Get() const { static_assert(sizeof(T) == 0, "Unsupported conversion for type"); }

	/** Resolves the world-space location. */
	FVector GetLocation() const;

	/** Resolves the world-space rotation as a quaternion. */
	FQuat GetQuat() const;

	/** Resolves the world-space rotation as a rotator. */
	FRotator GetRotation() const;

	/** Resolves the full world-space transform. */
	FTransform GetTransform() const;

	/** Resolves the local-space location (relative to actor or component if applicable). */
	FVector GetRelativeLocation() const;

	/** Resolves the local-space rotation as a quaternion. */
	FQuat GetRelativeQuat() const;

	/** Resolves the local-space rotation as a rotator. */
	FRotator GetRelativeRotation() const;

	/** Resolves the local-space transform. */
	FTransform GetRelativeTransform() const;

	/** Returns the referenced actor, if any. */
	const AActor* GetActor() const;

	/** Returns the referenced scene component, if any. */
	const USceneComponent* GetSceneComponent() const;

	/** Whether this transform source is valid. */
	bool IsValid() const { return SourceType != EKzTransformSourceType::Invalid; }

	/** Invalidates this transform source, clearing all references. */
	void Invalidate();

	/** Optimized network serialization. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	void operator=(const FKzTransformSource& Other)
	{
		SourceType = Other.SourceType;
		LiteralTransform = Other.LiteralTransform;
		SourceActor = Other.SourceActor;
		SourceComponent = Other.SourceComponent;
		SourceSocketName = Other.SourceSocketName;
	}

	bool operator==(const FKzTransformSource& Other) const;
	bool operator!=(const FKzTransformSource& Other) const;
};

// Supported getter specializations
template<> inline FVector    FKzTransformSource::Get() const { return GetLocation(); }
template<> inline FRotator   FKzTransformSource::Get() const { return GetRotation(); }
template<> inline FQuat      FKzTransformSource::Get() const { return GetQuat(); }
template<> inline FTransform FKzTransformSource::Get() const { return GetTransform(); }

template<>
struct TStructOpsTypeTraits<FKzTransformSource> : public TStructOpsTypeTraitsBase2<FKzTransformSource>
{
	enum
	{
		WithNetSerializer = true,
	};
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "CollisionShape.h"
#include "KzShape.generated.h"

class FPrimitiveDrawInterface;
class FMeshElementCollector;

/**
 * Base structure for all geometric shape types.
 * This struct is not intended to be instantiated directly.
 */
USTRUCT(BlueprintType, meta = (Hidden))
struct FKzShape
{
	GENERATED_BODY()

public:
	/** Returns true if this shape has zero extent (e.g. radius or half-size is zero). */
	virtual bool IsZeroExtent() const PURE_VIRTUAL(FKzShape::IsZeroExtent, return true;);

	/** Ensures this shape's parameters are valid and physically consistent. */
	virtual void Sanitize() {}

	/** Computes the world-space axis-aligned bounding box (AABB) for this shape. */
	virtual FBox GetBoundingBox(const FVector& Position, const FQuat& Orientation) const PURE_VIRTUAL(FKzShape::GetBoundingBox, return {};);

	/** Returns the closest point on (or inside) this shape to a given world-space point. */
	virtual FVector GetClosestPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const PURE_VIRTUAL(FKzShape::GetClosestPoint, return {};);

	/** Checks whether a world-space point lies inside (or on the surface of) this shape. */
	virtual bool IntersectsPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const PURE_VIRTUAL(FKzShape::IntersectsPoint, return false;);

	/** Returns an engine-level FCollisionShape representing this Kz shape. */
	virtual FCollisionShape ToCollisionShape(float Inflation = 0.0f) const PURE_VIRTUAL(FKzShape::ToCollisionShape, return {};);

	/** Uniformly inflates the shape by the specified amount. */
	virtual void Inflate(float Inflation) PURE_VIRTUAL(FKzShape::Inflate, ;);

	/** Inflates the shape by the specified per-axis amount. */
	virtual void Inflate(const FVector& Inflation) PURE_VIRTUAL(FKzShape::Inflate, ;);

	/** Uniformly scales the shape by the specified factor. */
	virtual void Scale(float Scale) PURE_VIRTUAL(FKzShape::Scale, ;);

	/** Scales the shape non-uniformly by the specified factor. */
	virtual void Scale(const FVector& Scale) PURE_VIRTUAL(FKzShape::Scale, ;);

	/** Draws a debug visualization of this shape. */
	virtual void DrawDebug(const UWorld* InWorld, FVector const& Position, const FQuat& Orientation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const PURE_VIRTUAL(FKzShape::DrawDebug, );

	/** Renders this shape using the given primitive draw interface. */
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const PURE_VIRTUAL(FKzShape::DrawSceneProxy, );

	/** Returns true if this shape provides a fast analytical raycast. */
	virtual bool ImplementsRaycast() const { return false; }

	/**
	 * Optional fast-path raycast against this shape.
	 * Should only be called if ImplementsRaycast() returns true.
	 */
	virtual bool Raycast(struct FKzHitResult& OutHit, const FVector& Position, const FQuat& Orientation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
	{
		return false; // base: no fast path
	}

	/** Returns the farthest point in the given direction, in local space. */
	virtual FVector GetSupportPoint(const FVector& Direction) const PURE_VIRTUAL(FKzShape::GetSupportPoint, return {};);
};
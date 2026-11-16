// Copyright 2026 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "KzPoint.generated.h"

/**
 * A point, with no size at all: tested against another shape, all it asks is whether it lies inside, which spatial queries answer without GJK.
 * Inflating or scaling it leaves it a point; whoever needs a ball wants a sphere.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Point"))
struct KZLIB_API FKzPoint : public FKzShape
{
	GENERATED_BODY()

	virtual bool IsZeroExtent() const override
	{
		return true;
	}

	virtual FBox GetBoundingBox(const FVector& Position, const FQuat& Rotation) const override
	{
		return FBox(Position, Position);
	}

	virtual FVector GetClosestPoint(const FVector& Position, const FQuat& Rotation, const FVector& Point) const override
	{
		return Position;
	}

	virtual bool IntersectsPoint(const FVector& Position, const FQuat& Rotation, const FVector& Point) const override
	{
		return Position.Equals(Point);
	}

	/** A sphere of the inflation: of none, which the engine traces as a line. */
	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeSphere(FMath::Max(0.0f, Inflation));
	}

	virtual void Inflate(float Inflation) override {}
	virtual void Inflate(const FVector& Inflation) override {}
	virtual void Scale(float Scale) override {}
	virtual void Scale(const FVector& Scale) override {}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		return FVector::ZeroVector;
	}

	/** Drawn as a cross along its axes. */
	virtual void DrawDebug(const UWorld* InWorld, FVector const& Position, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};
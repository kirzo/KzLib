// Copyright 2026 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "Math/Geometry/KzGeometry.h"
#include "KzWedge.generated.h"

/** A slice of a cylinder: a sector around the Z axis through the origin, opening along +X, for a horizontal aim or swing area. */
USTRUCT(BlueprintType, meta = (DisplayName = "Wedge"))
struct KZLIB_API FKzWedge : public FKzShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 200.0f;

	/** Angle from +X to each side. Up to 90, a half cylinder, the wedge stays convex. */
	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float HalfAngle = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float HalfHeight = 50.0f;

	FKzWedge() = default;

	explicit FKzWedge(float InRadius, float InHalfAngle, float InHalfHeight)
		: Radius(InRadius), HalfAngle(InHalfAngle), HalfHeight(InHalfHeight)
	{
		Sanitize();
	}

	virtual bool IsZeroExtent() const override
	{
		return Radius <= 0.0f || HalfAngle <= 0.0f || HalfHeight <= 0.0f;
	}

	virtual void Sanitize() override
	{
		Radius = FMath::Max(0.0f, Radius);
		HalfAngle = FMath::Clamp(HalfAngle, 0.0f, 90.0f);
		HalfHeight = FMath::Max(0.0f, HalfHeight);
	}

	virtual FBox GetBoundingBox(const FVector& Origin, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const override;

	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeBox(FVector(Radius, Radius, HalfHeight) + Inflation);
	}

	FORCEINLINE FKzWedge operator+(float Inflation) const
	{
		FKzWedge Result(*this);
		Result.Inflate(Inflation);
		return Result;
	}

	FORCEINLINE FKzWedge& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzWedge operator+(const FVector& Inflation) const
	{
		FKzWedge Result(*this);
		Result.Inflate(Inflation);
		return Result;
	}

	FORCEINLINE FKzWedge& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzWedge operator*(float Scale) const
	{
		FKzWedge Result(*this);
		Result.Scale(Scale);
		return Result;
	}

	FORCEINLINE FKzWedge& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzWedge operator*(const FVector& Scale) const
	{
		FKzWedge Result(*this);
		Result.Scale(Scale);
		return Result;
	}

	FORCEINLINE FKzWedge& operator*=(const FVector& Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	virtual void Inflate(float Inflation) override
	{
		Radius += Inflation;
		HalfHeight += Inflation;
		Sanitize();
	}

	virtual void Inflate(const FVector& Inflation) override
	{
		Radius += Inflation.X;
		HalfHeight += Inflation.Z;
		Sanitize();
	}

	virtual void Scale(float Scale) override
	{
		Radius *= FMath::Abs(Scale);
		HalfHeight *= FMath::Abs(Scale);
	}

	virtual void Scale(const FVector& Scale) override
	{
		const float Along = FMath::Abs(Scale.X);
		const float Across = FMath::Abs(Scale.Y);
		Radius *= FMath::Min(Along, Across);
		HalfHeight *= FMath::Abs(Scale.Z);

		// Stretching across +X more than along it opens the wedge; a half cylinder stays one.
		if (HalfAngle < 90.0f && Along > UE_KINDA_SMALL_NUMBER)
		{
			HalfAngle = FMath::RadiansToDegrees(FMath::Atan(FMath::Tan(FMath::DegreesToRadians(HalfAngle)) * Across / Along));
		}
		Sanitize();
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		return Kz::Geom::WedgeSupportPoint(Radius, HalfAngle, HalfHeight, Direction);
	}

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Origin, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzWedge operator+(float Inflation, const FKzWedge& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzWedge operator+(const FVector& Inflation, const FKzWedge& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzWedge operator*(float Scale, const FKzWedge& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzWedge operator*(const FVector& Scale, const FKzWedge& Shape)
{
	return Shape.operator*(Scale);
}
// Copyright 2026 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "Math/Geometry/KzGeometry.h"
#include "KzCone.generated.h"

/** A cone from its tip at the origin along +X. With a spherical cap it is a range and an angle, as a vision or aim cone; a tip radius truncates it. */
USTRUCT(BlueprintType, meta = (DisplayName = "Cone"))
struct KZLIB_API FKzCone : public FKzShape
{
	GENERATED_BODY()

	/** How far the cone reaches along its axis, whichever the cap. */
	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Length = 200.0f;

	/** Angle between the axis and the side. */
	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0, ClampMax = 89, Units = "Degrees"))
	float HalfAngle = 30.0f;

	/** Radius of the tip. Above zero the cone is truncated, with caps of two different radii. */
	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float TipRadius = 0.0f;

	/** Whether the far cap is a sphere around the apex, so that the axis and the rim reach as far, or flat. */
	UPROPERTY(EditAnywhere, Category = "Kz")
	bool bSphericalCap = true;

	FKzCone() = default;

	explicit FKzCone(float InLength, float InHalfAngle, float InTipRadius = 0.0f, bool bInSphericalCap = true)
		: Length(InLength), HalfAngle(InHalfAngle), TipRadius(InTipRadius), bSphericalCap(bInSphericalCap)
	{
		Sanitize();
	}

	virtual bool IsZeroExtent() const override
	{
		return Length <= 0.0f || (HalfAngle <= 0.0f && TipRadius <= 0.0f);
	}

	virtual void Sanitize() override
	{
		Length = FMath::Max(0.0f, Length);
		HalfAngle = FMath::Clamp(HalfAngle, 0.0f, Kz::Geom::MaxConeHalfAngle);
		TipRadius = FMath::Max(0.0f, TipRadius);
	}

	virtual FBox GetBoundingBox(const FVector& Origin, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Origin, const FQuat& Rotation, const FVector& Point) const override;
	virtual FCollisionShape ToCollisionShape(float Inflation) const override;

	FORCEINLINE FKzCone operator+(float Inflation) const
	{
		FKzCone Result(*this);
		Result.Inflate(Inflation);
		return Result;
	}

	FORCEINLINE FKzCone& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCone operator+(const FVector& Inflation) const
	{
		FKzCone Result(*this);
		Result.Inflate(Inflation);
		return Result;
	}

	FORCEINLINE FKzCone& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCone operator*(float Scale) const
	{
		FKzCone Result(*this);
		Result.Scale(Scale);
		return Result;
	}

	FORCEINLINE FKzCone& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzCone operator*(const FVector& Scale) const
	{
		FKzCone Result(*this);
		Result.Scale(Scale);
		return Result;
	}

	FORCEINLINE FKzCone& operator*=(const FVector& Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	virtual void Inflate(float Inflation) override
	{
		Length += Inflation;
		TipRadius += Inflation;
		Sanitize();
	}

	virtual void Inflate(const FVector& Inflation) override
	{
		Length += Inflation.X;
		TipRadius += FMath::Min(Inflation.Y, Inflation.Z);
		Sanitize();
	}

	virtual void Scale(float Scale) override
	{
		Length *= FMath::Abs(Scale);
		TipRadius *= FMath::Abs(Scale);
	}

	virtual void Scale(const FVector& Scale) override
	{
		const float Along = FMath::Abs(Scale.X);
		const float Across = FMath::Min(FMath::Abs(Scale.Y), FMath::Abs(Scale.Z));
		Length *= Along;
		TipRadius *= Across;

		// Stretching across the axis more than along it opens the cone.
		if (Along > UE_KINDA_SMALL_NUMBER)
		{
			HalfAngle = FMath::RadiansToDegrees(FMath::Atan(FMath::Tan(FMath::DegreesToRadians(HalfAngle)) * Across / Along));
		}
		Sanitize();
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		return Kz::Geom::ConeSupportPoint(Length, HalfAngle, TipRadius, bSphericalCap, Direction);
	}

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Origin, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzCone operator+(float Inflation, const FKzCone& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCone operator+(const FVector& Inflation, const FKzCone& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCone operator*(float Scale, const FKzCone& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzCone operator*(const FVector& Scale, const FKzCone& Shape)
{
	return Shape.operator*(Scale);
}
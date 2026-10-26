// Copyright 2025 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "KzCylinder.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Cylinder"))
struct KZLIB_API FKzCylinder : public FKzShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float HalfHeight = 100.0f;

	FKzCylinder() = default;

	explicit FKzCylinder(float InRadius, float InHalfHeight)
		: Radius(InRadius), HalfHeight(InHalfHeight)
	{
		Sanitize();
	}

	virtual bool IsZeroExtent() const override
	{
		return Radius <= 0.0f || HalfHeight <= 0.0f;
	}

	virtual void Sanitize() override
	{
		Radius = FMath::Max(0.0f, Radius);
		HalfHeight = FMath::Max(0.0f, HalfHeight);
	}

	virtual FBox GetBoundingBox(const FVector& Center, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;

	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeBox(FVector(Radius, Radius, HalfHeight) + Inflation);
	}

	FORCEINLINE FKzCylinder operator+(float Inflation) const
	{
		return FKzCylinder(Radius + Inflation, HalfHeight + Inflation);
	}

	FORCEINLINE FKzCylinder& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCylinder operator+(const FVector& Inflation) const
	{
		return FKzCylinder(Radius + Inflation.X, HalfHeight + Inflation.Z);
	}

	FORCEINLINE FKzCylinder& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCylinder operator*(float Scale) const
	{
		return FKzCylinder(Radius * FMath::Abs(Scale), HalfHeight * FMath::Abs(Scale));
	}

	FORCEINLINE FKzCylinder& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzCylinder operator*(const FVector& Scale) const
	{
		return FKzCylinder(Radius * FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y)), HalfHeight * FMath::Abs(Scale.Z));
	}

	FORCEINLINE FKzCylinder& operator*=(const FVector& Scale)
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
		Radius *= FMath::Max(FMath::Abs(Scale.X), FMath::Abs(Scale.Y));
		HalfHeight *= FMath::Abs(Scale.Z);
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		const float LenSqXY = Direction.SizeSquared2D();
		if (LenSqXY < UE_KINDA_SMALL_NUMBER)
		{
			return FVector(0.0f, 0.0f, FMath::Sign(Direction.Z) * HalfHeight);
		}

		const float Scale = Radius * FMath::InvSqrt(LenSqXY);
		return FVector(Direction.X * Scale, Direction.Y * Scale, FMath::Sign(Direction.Z) * HalfHeight);
	}

	virtual bool ImplementsRaycast() const override { return true; }
	virtual bool Raycast(struct FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const override;

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzCylinder operator+(float Inflation, const FKzCylinder& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCylinder operator+(const FVector& Inflation, const FKzCylinder& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCylinder operator*(float Scale, const FKzCylinder& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzCylinder operator*(const FVector& Scale, const FKzCylinder& Shape)
{
	return Shape.operator*(Scale);
}

#undef SHAPE_OPERATORS
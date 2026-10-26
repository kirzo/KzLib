// Copyright 2025 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "KzSphere.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Sphere"))
struct KZLIB_API FKzSphere : public FKzShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	FKzSphere() = default;

	explicit FKzSphere(float InRadius)
		: Radius(InRadius)
	{
		Sanitize();
	}

	virtual bool IsZeroExtent() const override
	{
		return Radius <= 0.0f;
	}

	virtual void Sanitize() override
	{
		Radius = FMath::Max(0.0f, Radius);
	}

	virtual FBox GetBoundingBox(const FVector& Center, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;

	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeSphere(Radius + Inflation);
	}

	FORCEINLINE FKzSphere operator+(float Inflation) const
	{
		return FKzSphere(Radius + Inflation);
	}

	FORCEINLINE FKzSphere& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzSphere operator+(const FVector& Inflation) const
	{
		return FKzSphere(Radius + Inflation.X);
	}

	FORCEINLINE FKzSphere& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzSphere operator*(float Scale) const
	{
		return FKzSphere(Radius * FMath::Abs(Scale));
	}

	FORCEINLINE FKzSphere& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzSphere operator*(const FVector& Scale) const
	{
		return FKzSphere(Radius * Scale.GetAbsMin());
	}

	FORCEINLINE FKzSphere& operator*=(const FVector& Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	virtual void Inflate(float Inflation) override
	{
		Radius += Inflation;
		Sanitize();
	}

	virtual void Inflate(const FVector& Inflation) override
	{
		Radius += Inflation.X;
		Sanitize();
	}

	virtual void Scale(float Scale) override
	{
		Radius *= FMath::Abs(Scale);
	}

	virtual void Scale(const FVector& Scale) override
	{
		Radius *= Scale.GetAbsMin();
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		return Direction.GetSafeNormal() * Radius;
	}

	virtual bool ImplementsRaycast() const override { return true; }
	virtual bool Raycast(struct FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const override;

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzSphere operator+(float Inflation, const FKzSphere& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzSphere operator+(const FVector& Inflation, const FKzSphere& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzSphere operator*(float Scale, const FKzSphere& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzSphere operator*(const FVector& Scale, const FKzSphere& Shape)
{
	return Shape.operator*(Scale);
}
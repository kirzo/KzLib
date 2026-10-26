// Copyright 2025 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "KzCapsule.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Capsule"))
struct KZLIB_API FKzCapsule : public FKzShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float HalfHeight = 100.0f;

	FKzCapsule() = default;

	explicit FKzCapsule(float InRadius, float InHalfHeight)
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
		HalfHeight = FMath::Max(0.0f, HalfHeight);
		Radius = FMath::Clamp(Radius, 0.0f, HalfHeight);
	}

	virtual FBox GetBoundingBox(const FVector& Center, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;

	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeCapsule(Radius + Inflation, HalfHeight + Inflation);
	}

	FORCEINLINE FKzCapsule operator+(float Inflation) const
	{
		return FKzCapsule(Radius + Inflation, HalfHeight + Inflation);
	}

	FORCEINLINE FKzCapsule& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCapsule operator+(const FVector& Inflation) const
	{
		return FKzCapsule(Radius + Inflation.X, HalfHeight + Inflation.Z);
	}

	FORCEINLINE FKzCapsule& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzCapsule operator*(float Scale) const
	{
		return FKzCapsule(Radius * FMath::Abs(Scale), HalfHeight * FMath::Abs(Scale));
	}

	FORCEINLINE FKzCapsule& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzCapsule operator*(const FVector& Scale) const
	{
		return FKzCapsule(Radius * FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y)), HalfHeight * FMath::Abs(Scale.Z));
	}

	FORCEINLINE FKzCapsule& operator*=(const FVector& Scale)
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
		Radius = +Inflation.X;
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
		Radius *= FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y));
		HalfHeight *= FMath::Abs(Scale.Z);
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		if (Direction.IsNearlyZero())
		{
			return FVector(0.0f, 0.0f, HalfHeight);
		}

		const float LenSqXY = Direction.SizeSquared2D();
		const float HemisphereZ = (HalfHeight - Radius) * FMath::Sign(Direction.Z);

		FVector SphereSupport;

		if (LenSqXY < UE_KINDA_SMALL_NUMBER)
		{
			SphereSupport = FVector(0.0f, 0.0f, Radius * FMath::Sign(Direction.Z));
		}
		else
		{
			const float Scale = Radius * FMath::InvSqrt(LenSqXY);
			SphereSupport = FVector(Direction.X * Scale, Direction.Y * Scale, Radius * FMath::Sign(Direction.Z));
		}

		return FVector(SphereSupport.X, SphereSupport.Y, SphereSupport.Z + HemisphereZ);
	}

	virtual bool ImplementsRaycast() const override { return true; }
	virtual bool Raycast(struct FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const override;

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzCapsule operator+(float Inflation, const FKzCapsule& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCapsule operator+(const FVector& Inflation, const FKzCapsule& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzCapsule operator*(float Scale, const FKzCapsule& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzCapsule operator*(const FVector& Scale, const FKzCapsule& Shape)
{
	return Shape.operator*(Scale);
}
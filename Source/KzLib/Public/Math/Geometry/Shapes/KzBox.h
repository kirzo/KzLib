// Copyright 2025 kirzo

#pragma once

#include "Math/Geometry/KzShape.h"
#include "KzBox.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Box"))
struct KZLIB_API FKzBox : public FKzShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Kz")
	FVector HalfSize = FVector(50.0f);

	FKzBox() = default;

	explicit FKzBox(const FVector& InHalfSize)
		: HalfSize(InHalfSize)
	{
		Sanitize();
	}

	virtual bool IsZeroExtent() const override
	{
		return HalfSize.X <= 0.0f || HalfSize.Y <= 0.0f || HalfSize.Z <= 0.0f;
	}

	virtual void Sanitize() override
	{
		HalfSize.X = FMath::Max(0.0f, HalfSize.X);
		HalfSize.Y = FMath::Max(0.0f, HalfSize.Y);
		HalfSize.Z = FMath::Max(0.0f, HalfSize.Z);
	}

	virtual FBox GetBoundingBox(const FVector& Center, const FQuat& Rotation) const override;
	virtual FVector GetClosestPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;
	virtual bool IntersectsPoint(const FVector& Center, const FQuat& Rotation, const FVector& Point) const override;

	virtual FCollisionShape ToCollisionShape(float Inflation) const override
	{
		return FCollisionShape::MakeBox(HalfSize + Inflation);
	}

	FORCEINLINE FKzBox operator+(float Inflation) const
	{
		return FKzBox(HalfSize + Inflation);
	}

	FORCEINLINE FKzBox& operator+=(float Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzBox operator+(const FVector& Inflation) const
	{
		return FKzBox(HalfSize + Inflation);
	}

	FORCEINLINE FKzBox& operator+=(const FVector& Inflation)
	{
		this->Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzBox operator*(float Scale) const
	{
		return FKzBox(HalfSize * FMath::Abs(Scale));
	}

	FORCEINLINE FKzBox& operator*=(float Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzBox operator*(const FVector& Scale) const
	{
		return FKzBox(HalfSize * Scale.GetAbs());
	}

	FORCEINLINE FKzBox& operator*=(const FVector& Scale)
	{
		this->Scale(Scale);
		return *this;
	}

	virtual void Inflate(float Inflation) override
	{
		HalfSize += FVector(Inflation);
		Sanitize();
	}

	virtual void Inflate(const FVector& Inflation) override
	{
		HalfSize += Inflation;
		Sanitize();
	}

	virtual void Scale(float Scale) override
	{
		HalfSize *= FMath::Abs(Scale);
	}

	virtual void Scale(const FVector& Scale) override
	{
		HalfSize *= Scale.GetAbs();
	}

	virtual FVector GetSupportPoint(const FVector& Direction) const override
	{
		return FVector(
			Direction.X >= 0.0f ? HalfSize.X : -HalfSize.X,
			Direction.Y >= 0.0f ? HalfSize.Y : -HalfSize.Y,
			Direction.Z >= 0.0f ? HalfSize.Z : -HalfSize.Z
		);
	}

	virtual bool ImplementsRaycast() const override { return true; }
	virtual bool Raycast(struct FKzHitResult& OutHit, const FVector& Center, const FQuat& Rotation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const override;

	virtual void DrawDebug(const UWorld* InWorld, FVector const& Center, const FQuat& Rotation, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) const override;
	virtual void DrawSceneProxy(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, bool bDrawSolid, float Thickness, int32 ViewIndex, FMeshElementCollector& Collector) const override;
	virtual void DrawWire(FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, const FLinearColor& Color, float Thickness, uint8 DepthPriority) const override;
};

FORCEINLINE FKzBox operator+(float Inflation, const FKzBox& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzBox operator+(const FVector& Inflation, const FKzBox& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzBox operator*(float Scale, const FKzBox& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzBox operator*(const FVector& Scale, const FKzBox& Shape)
{
	return Shape.operator*(Scale);
}
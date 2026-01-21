// Copyright 2026 kirzo

#pragma once

#include "KzShape.h"
#include "StructUtils/InstancedStruct.h"
#include "KzShapeInstance.generated.h"

/**
 * Defines a specific geometric shape instance with its parameters.
 *
 * Used to describe individual shapes used in queries, overlaps, or procedural geometry operations.
 */
USTRUCT(BlueprintType, meta = (ShowOnlyInnerProperties))
struct KZLIB_API FKzShapeInstance
{
	GENERATED_BODY()

	template<typename T>
	using TEnableIfShape = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>;

private:
	/** The underlying instanced shape definition. */
	UPROPERTY(EditAnywhere, NoClear, Category = "Shape", meta = (ExcludeBaseStruct))
	TInstancedStruct<FKzShape> Shape;

public:
	FKzShapeInstance();

	FKzShapeInstance(ENoInit) {}

	FKzShapeInstance(const FKzShapeInstance& Other)
		: Shape(Other.Shape)
	{
	}

	FKzShapeInstance(FKzShapeInstance&& Other) noexcept
		: Shape(MoveTemp(Other.Shape))
	{
	}

	FORCEINLINE FKzShapeInstance& operator=(const FKzShapeInstance& Other)
	{
		if (this != &Other)
		{
			Shape = Other.Shape;
		}
		return *this;
	}

	FORCEINLINE FKzShapeInstance& operator=(FKzShapeInstance&& Other) noexcept
	{
		if (this != &Other)
		{
			Shape = MoveTemp(Other.Shape);
		}
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator+(float Inflation) const
	{
		FKzShapeInstance Result(*this);
		Result += Inflation;
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator+=(float Inflation)
	{
		Shape.GetMutable().Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator+(const FVector& Inflation) const
	{
		FKzShapeInstance Result(*this);
		Result += Inflation;
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator+=(const FVector& Inflation)
	{
		Shape.GetMutable().Inflate(Inflation);
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator*(float Scale) const
	{
		FKzShapeInstance Result(*this);
		Result *= Scale;
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator*=(float Scale)
	{
		Shape.GetMutable().Scale(Scale);
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator*(const FVector& Scale) const
	{
		FKzShapeInstance Result(*this);
		Result *= Scale;
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator*=(const FVector& Scale)
	{
		Shape.GetMutable().Scale(Scale);
		return *this;
	}

	template<typename T, typename = TEnableIfShape<T>>
	FKzShapeInstance(const T& Struct)
	{
		Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), reinterpret_cast<const uint8*>(&Struct));
	}

	/** Initializes from struct type and emplace construct. */
	template<typename T, typename... TArgs, typename = TEnableIfShape<T>>
	void InitializeAs(TArgs&&... InArgs)
	{
		Shape.InitializeAs<T>(Forward<TArgs>(InArgs)...);
	}

	/** Creates a new FKzShapeInstance from templated struct type. */
	template<typename T, typename = TEnableIfShape<T>>
	static FKzShapeInstance Make()
	{
		FKzShapeInstance This;
		This.Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), nullptr);
		return This;
	}

	/** Creates a new FKzShapeInstance from templated struct type. */
	template<typename T, typename = TEnableIfShape<T>>
	static FKzShapeInstance Make(const T& Struct)
	{
		FKzShapeInstance This;
		This.Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), reinterpret_cast<const uint8*>(&Struct));
		return This;
	}

	/** Creates a new FKzShapeInstance from the templated type and forward all arguments to constructor. */
	template<typename T, typename... TArgs, typename = TEnableIfShape<T>>
	static FKzShapeInstance Make(TArgs&&... InArgs)
	{
		FKzShapeInstance This;
		This.InitializeAs<T>(Forward<TArgs>(InArgs)...);
		return This;
	}

	/**
	 * Returns true if the contained shape is exactly of the specified type.
	 *
	 * Unlike IsA(), this performs a strict type comparison (no inheritance check).
	 */
	template<typename T, typename = TEnableIfShape<T>>
	FORCEINLINE bool IsOfType() const
	{
		return IsValid() && T::StaticStruct() == Shape.GetScriptStruct();
	}

	/** Returns true if the contained shape is of the specified type or a derived type. */
	template<typename T, typename = TEnableIfShape<T>>
	FORCEINLINE bool IsA() const
	{
		const UScriptStruct* Struct = Shape.GetScriptStruct();
		return Struct && Struct->IsChildOf(T::StaticStruct());
	}

	/** Templated accessor for retrieving the shape as a concrete derived type. */
	template<typename T, typename = TEnableIfShape<T>>
	FORCEINLINE const T& As() const
	{
		checkf(IsA<T>(), TEXT("Shape is not of type %s"), *T::StaticStruct()->GetName());
		return Shape.Get<T>();
	}

	/** Templated accessor for retrieving the shape as a concrete derived type. */
	template<typename T, typename = TEnableIfShape<T>>
	FORCEINLINE T& As()
	{
		checkf(IsA<T>(), TEXT("Shape is not of type %s"), *T::StaticStruct()->GetName());
		return Shape.GetMutable<T>();
	}

	/**
	 * Templated accessor for retrieving the shape as a concrete derived type.
	 * Returns nullptr if type doesn't match.
	 */
	template<typename T, typename = TEnableIfShape<T>>
	const T* TryAs() const
	{
		return Shape.GetPtr<T>();
	}

	/**
	 * Templated accessor for retrieving the shape as a concrete derived type.
	 * Returns nullptr if type doesn't match.
	 */
	template<typename T, typename = TEnableIfShape<T>>
	T* TryAs()
	{
		return Shape.GetMutablePtr<T>();
	}

	/** Returns true if the contained shape instance is valid. */
	FORCEINLINE bool IsValid() const { return Shape.IsValid(); }

	/** Returns true if this shape has zero extent (e.g. radius or half-size is zero). */
	FORCEINLINE bool IsZeroExtent() const { return IsValid() ? Shape.Get().IsZeroExtent() : true; }

	/** Computes the world-space axis-aligned bounding box (AABB) for this shape. */
	FORCEINLINE FBox GetBoundingBox(const FTransform& Transform) const
	{
		return GetBoundingBox(Transform.GetLocation(), Transform.GetRotation());
	}

	/** Computes the world-space axis-aligned bounding box (AABB) for this shape. */
	FORCEINLINE FBox GetBoundingBox(const FVector& Position, const FQuat& Orientation) const
	{
		return IsValid() ? Shape.Get().GetBoundingBox(Position, Orientation) : FBox(ForceInitToZero);
	}

	/** Returns an engine-level FCollisionShape representing this Kz shape. */
	FORCEINLINE FCollisionShape ToCollisionShape(float Inflation = 0.0f) const
	{
		return IsValid() ? Shape.Get().ToCollisionShape(Inflation) : FCollisionShape::MakeBox(FVector::ZeroVector);
	}

	/** Returns the closest point on (or inside) this shape to a given world-space point. */
	FORCEINLINE FVector GetClosestPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const
	{
		return IsValid() ? Shape.Get().GetClosestPoint(Position, Orientation, Point) : FVector::ZeroVector;
	}

	/** Checks whether a world-space point lies inside (or on the surface of) this shape. */
	FORCEINLINE bool IntersectsPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const
	{
		return IsValid() ? Shape.Get().IntersectsPoint(Position, Orientation, Point) : false;
	}

	/** Returns true if this shape provides a fast analytical raycast. */
	FORCEINLINE bool ImplementsRaycast() const
	{
		return IsValid() ? Shape.Get().ImplementsRaycast() : false;
	}

	/**
	 * Optional fast-path raycast against this shape.
	 * Should only be called if ImplementsRaycast() returns true.
	 */
	FORCEINLINE bool Raycast(struct FKzHitResult& OutHit, const FVector& Position, const FQuat& Orientation, const FVector& RayStart, const FVector& RayDir, float MaxDistance) const
	{
		return IsValid() ? Shape.Get().Raycast(OutHit, Position, Orientation, RayStart, RayDir, MaxDistance) : false;
	}

	/** Returns the farthest point in the given direction, in local space. */
	FORCEINLINE FVector GetSupportPoint(const FVector& Direction) const
	{
		return IsValid() ? Shape.Get().GetSupportPoint(Direction) : FVector::ZeroVector;
	}
};

FORCEINLINE FKzShapeInstance operator+(float Inflation, const FKzShapeInstance& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzShapeInstance operator+(const FVector& Inflation, const FKzShapeInstance& Shape)
{
	return Shape.operator+(Inflation);
}

FORCEINLINE FKzShapeInstance operator*(float Scale, const FKzShapeInstance& Shape)
{
	return Shape.operator*(Scale);
}

FORCEINLINE FKzShapeInstance operator*(const FVector& Scale, const FKzShapeInstance& Shape)
{
	return Shape.operator*(Scale);
}
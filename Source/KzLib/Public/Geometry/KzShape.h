// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "KzShape.generated.h"

UENUM()
enum class EKzShapeType : uint8
{
	Sphere,
	Box,
	Capsule,
	Cylinder
};

/**
 * Base structure for all geometric shape types.
 * This struct is not intended to be instantiated directly.
 */
USTRUCT(BlueprintType, meta = (Hidden))
struct FKzShape
{
	GENERATED_BODY()

public:
	/** Returns the underlying shape type. */
	EKzShapeType GetType() const { return Type; }

	/** Returns true if this shape has zero extent (e.g. radius or half-size is zero). */
	virtual bool IsZeroExtent() const PURE_VIRTUAL(FKzShape::IsZeroExtent, return true;);

	/**
	 * Ensures this shape's parameters are valid and physically consistent.
	 * Called to correct any invalid or out-of-range values (e.g., negative dimensions or inconsistent ratios).
	 */
	virtual void Sanitize() {}

	/** Returns an engine-level FCollisionShape representing this Kz shape. */
	virtual struct FCollisionShape ToCollisionShape(float Inflation = 0.0f) const PURE_VIRTUAL(FKzShape::GetAABB, return {};);

protected:
		/** The internal shape type that identifies the derived shape variant. */
	UPROPERTY(BlueprintReadOnly)
	EKzShapeType Type = EKzShapeType::Sphere;
};

#define SHAPE_TYPE_BOILERPLATE(ShapeType, EnumType) \
private: \
	using Super = FKzShape; \
	using Super::FKzShape; /* Base constructors private */ \
	\
public: \
	static constexpr EKzShapeType StaticType = EnumType; \
	ShapeType() { Type = StaticType; } \
	KZLIB_API virtual FCollisionShape ToCollisionShape(float Inflation) const override;

USTRUCT(BlueprintType, meta = (DisplayName = "Sphere"))
struct FKzSphere : public FKzShape
{
	GENERATED_BODY()
	SHAPE_TYPE_BOILERPLATE(FKzSphere, EKzShapeType::Sphere)

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	explicit FKzSphere(float InRadius)
		: Radius(InRadius)
	{
		Type = StaticType;
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

	FORCEINLINE FKzSphere operator+(float Inflation) const
	{
		return FKzSphere(Radius + Inflation);
	}

	FORCEINLINE FKzSphere& operator+=(float Inflation)
	{
		Radius += Inflation;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzSphere operator+(const FVector& Inflation) const
	{
		return FKzSphere(Radius + Inflation.X);
	}

	FORCEINLINE FKzSphere& operator+=(const FVector& Inflation)
	{
		Radius += Inflation.X;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzSphere operator*(float Scale) const
	{
		return FKzSphere(Radius * FMath::Abs(Scale));
	}

	FORCEINLINE FKzSphere& operator*=(float Scale)
	{
		Radius *= FMath::Abs(Scale);
		return *this;
	}

	FORCEINLINE FKzSphere operator*(const FVector& Scale) const
	{
		return FKzSphere(Radius * Scale.GetAbsMin());
	}

	FORCEINLINE FKzSphere& operator*=(const FVector& Scale)
	{
		Radius *= Scale.GetAbsMin();
		return *this;
	}
};

USTRUCT(BlueprintType, meta = (DisplayName = "Box"))
struct FKzBox : public FKzShape
{
	GENERATED_BODY()
	SHAPE_TYPE_BOILERPLATE(FKzBox, EKzShapeType::Box)

	UPROPERTY(EditAnywhere, Category = "Kz")
	FVector HalfSize = FVector(50.0f);

	explicit FKzBox(const FVector& InHalfSize)
		: HalfSize(InHalfSize)
	{
		Type = StaticType;
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

	FORCEINLINE FKzBox operator+(float Inflation) const
	{
		return FKzBox(HalfSize + Inflation);
	}

	FORCEINLINE FKzBox& operator+=(float Inflation)
	{
		HalfSize += FVector(Inflation);
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzBox operator+(const FVector& Inflation) const
	{
		return FKzBox(HalfSize + Inflation);
	}

	FORCEINLINE FKzBox& operator+=(const FVector& Inflation)
	{
		HalfSize += Inflation;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzBox operator*(float Scale) const
	{
		return FKzBox(HalfSize * FMath::Abs(Scale));
	}

	FORCEINLINE FKzBox& operator*=(float Scale)
	{
		HalfSize *= FMath::Abs(Scale);
		return *this;
	}

	FORCEINLINE FKzBox operator*(const FVector& Scale) const
	{
		return FKzBox(HalfSize * Scale.GetAbs());
	}

	FORCEINLINE FKzBox& operator*=(const FVector& Scale)
	{
		HalfSize *= Scale.GetAbs();
		return *this;
	}
};

USTRUCT(BlueprintType, meta = (DisplayName = "Capsule"))
struct FKzCapsule : public FKzShape
{
	GENERATED_BODY()
	SHAPE_TYPE_BOILERPLATE(FKzCapsule, EKzShapeType::Capsule)

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float HalfHeight = 100.0f;

	explicit FKzCapsule(float InRadius, float InHalfHeight)
		: Radius(InRadius), HalfHeight(InHalfHeight)
	{
		Type = StaticType;
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

	FORCEINLINE FKzCapsule operator+(float Inflation) const
	{
		return FKzCapsule(Radius + Inflation, HalfHeight + Inflation);
	}

	FORCEINLINE FKzCapsule& operator+=(float Inflation)
	{
		Radius += Inflation;
		HalfHeight += Inflation;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzCapsule operator+(const FVector& Inflation) const
	{
		return FKzCapsule(Radius + Inflation.X, HalfHeight + Inflation.Z);
	}

	FORCEINLINE FKzCapsule& operator+=(const FVector& Inflation)
	{
		Radius =+ Inflation.X;
		HalfHeight += Inflation.Z;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzCapsule operator*(float Scale) const
	{
		return FKzCapsule(Radius * FMath::Abs(Scale), HalfHeight * FMath::Abs(Scale));
	}

	FORCEINLINE FKzCapsule& operator*=(float Scale)
	{
		Radius *= FMath::Abs(Scale);
		HalfHeight *= FMath::Abs(Scale);
		return *this;
	}

	FORCEINLINE FKzCapsule operator*(const FVector& Scale) const
	{
		return FKzCapsule(Radius * FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y)), HalfHeight * FMath::Abs(Scale.Z));
	}

	FORCEINLINE FKzCapsule& operator*=(const FVector& Scale)
	{
		Radius *= FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y));
		HalfHeight *= FMath::Abs(Scale.Z);
		Sanitize();
		return *this;
	}
};

USTRUCT(BlueprintType, meta = (DisplayName = "Cylinder"))
struct FKzCylinder : public FKzShape
{
	GENERATED_BODY()
	SHAPE_TYPE_BOILERPLATE(FKzCylinder, EKzShapeType::Cylinder)

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float Radius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Kz", meta = (ClampMin = 0))
	float HalfHeight = 100.0f;

	explicit FKzCylinder(float InRadius, float InHalfHeight)
		: Radius(InRadius), HalfHeight(InHalfHeight)
	{
		Type = StaticType;
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

	FORCEINLINE FKzCylinder operator+(float Inflation) const
	{
		return FKzCylinder(Radius + Inflation, HalfHeight + Inflation);
	}

	FORCEINLINE FKzCylinder& operator+=(float Inflation)
	{
		Radius += Inflation;
		HalfHeight += Inflation;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzCylinder operator+(const FVector& Inflation) const
	{
		return FKzCylinder(Radius + Inflation.X, HalfHeight + Inflation.Z);
	}

	FORCEINLINE FKzCylinder& operator+=(const FVector& Inflation)
	{
		Radius += Inflation.X;
		HalfHeight += Inflation.Z;
		Sanitize();
		return *this;
	}

	FORCEINLINE FKzCylinder operator*(float Scale) const
	{
		return FKzCylinder(Radius * FMath::Abs(Scale), HalfHeight * FMath::Abs(Scale));
	}

	FORCEINLINE FKzCylinder& operator*=(float Scale)
	{
		Radius *= FMath::Abs(Scale);
		HalfHeight *= FMath::Abs(Scale);
		return *this;
	}

	FORCEINLINE FKzCylinder operator*(const FVector& Scale) const
	{
		return FKzCylinder(Radius * FMath::Min(FMath::Abs(Scale.X), FMath::Abs(Scale.Y)), HalfHeight * FMath::Abs(Scale.Z));
	}

	FORCEINLINE FKzCylinder& operator*=(const FVector& Scale)
	{
		Radius *= FMath::Max(FMath::Abs(Scale.X), FMath::Abs(Scale.Y));
		HalfHeight *= FMath::Abs(Scale.Z);
		return *this;
	}
};

#undef SHAPE_TYPE_BOILERPLATE

/**
 * Base structure for all geometric shape types.
 */
USTRUCT(BlueprintType, meta = (ShowOnlyInnerProperties))
struct KZLIB_API FKzShapeInstance
{
	GENERATED_BODY()

private:
	/** The underlying instanced shape definition. */
	UPROPERTY(EditAnywhere, NoClear, meta = (ExcludeBaseStruct))
	TInstancedStruct<FKzShape> Shape;

public:
	FKzShapeInstance()
	{
		Shape.InitializeAs<FKzSphere>();
	}

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

	template<typename Predicate>
	FORCEINLINE void WithShape(Predicate&& Pred) const
	{
		if (!IsValid())
			return;

		switch (GetType())
		{
			case EKzShapeType::Sphere:   Forward<Predicate>(Pred)(Shape.Get<FKzSphere>()); break;
			case EKzShapeType::Box:      Forward<Predicate>(Pred)(Shape.Get<FKzBox>()); break;
			case EKzShapeType::Capsule:  Forward<Predicate>(Pred)(Shape.Get<FKzCapsule>()); break;
			case EKzShapeType::Cylinder: Forward<Predicate>(Pred)(Shape.Get<FKzCylinder>()); break;
			default: break;
		}
	}

	template<typename Predicate>
	FORCEINLINE void WithShape(Predicate&& Pred)
	{
		if (!IsValid())
			return;

		switch (GetType())
		{
			case EKzShapeType::Sphere:   Forward<Predicate>(Pred)(Shape.GetMutable<FKzSphere>()); break;
			case EKzShapeType::Box:      Forward<Predicate>(Pred)(Shape.GetMutable<FKzBox>()); break;
			case EKzShapeType::Capsule:  Forward<Predicate>(Pred)(Shape.GetMutable<FKzCapsule>()); break;
			case EKzShapeType::Cylinder: Forward<Predicate>(Pred)(Shape.GetMutable<FKzCylinder>()); break;
			default: break;
		}
	}

	FORCEINLINE FKzShapeInstance operator+(float Inflation) const
	{
		FKzShapeInstance Result(NoInit);
		WithShape([&](const auto& TypedShape)
		{
			using TShape = std::decay_t<decltype(TypedShape)>;
			Result = FKzShapeInstance::Make<TShape>(TypedShape + Inflation);
		});
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator+=(float Inflation)
	{
		WithShape([&](auto& TypedShape) { TypedShape += Inflation; });
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator+(const FVector& Inflation) const
	{
		FKzShapeInstance Result(NoInit);
		WithShape([&](const auto& TypedShape)
		{
			using TShape = std::decay_t<decltype(TypedShape)>;
			Result = FKzShapeInstance::Make<TShape>(TypedShape + Inflation);
		});
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator+=(const FVector& Inflation)
	{
		WithShape([&](auto& TypedShape) { TypedShape += Inflation; });
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator*(float Scale) const
	{
		FKzShapeInstance Result(NoInit);
		WithShape([&](const auto& TypedShape)
		{
			using TShape = std::decay_t<decltype(TypedShape)>;
			Result = FKzShapeInstance::Make<TShape>(TypedShape * Scale);
		});
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator*=(float Scale)
	{
		WithShape([&](auto& TypedShape) { TypedShape *= Scale; });
		return *this;
	}

	FORCEINLINE FKzShapeInstance operator*(const FVector& Scale) const
	{
		FKzShapeInstance Result(NoInit);
		WithShape([&](const auto& TypedShape)
		{
			using TShape = std::decay_t<decltype(TypedShape)>;
			Result = FKzShapeInstance::Make<TShape>(TypedShape * Scale);
		});
		return Result;
	}

	FORCEINLINE FKzShapeInstance& operator*=(const FVector& Scale)
	{
		WithShape([&](auto& TypedShape) { TypedShape *= Scale; });
		return *this;
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>>
	FKzShapeInstance(const T& Struct)
	{
		Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), reinterpret_cast<const uint8*>(&Struct));
	}

	/** Initializes from struct type and emplace construct. */
	template<typename T, typename... TArgs, typename = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>>
	void InitializeAs(TArgs&&... InArgs)
	{
		Shape.InitializeAs<T>(Forward<TArgs>(InArgs)...);
	}

	/** Creates a new FKzShapeInstance from templated struct type. */
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>>
	static FKzShapeInstance Make()
	{
		FKzShapeInstance This;
		This.Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), nullptr);
		return This;
	}

	/** Creates a new FKzShapeInstance from templated struct type. */
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>>
	static FKzShapeInstance Make(const T& Struct)
	{
		FKzShapeInstance This;
		This.Shape.InitializeAsScriptStruct(TBaseStructure<T>::Get(), reinterpret_cast<const uint8*>(&Struct));
		return This;
	}

	/** Creates a new FKzShapeInstance from the templated type and forward all arguments to constructor. */
	template<typename T, typename... TArgs, typename = std::enable_if_t<TIsDerivedFrom<T, FKzShape>::Value>>
	static FKzShapeInstance Make(TArgs&&... InArgs)
	{
		FKzShapeInstance This;
		This.InitializeAs<T>(Forward<TArgs>(InArgs)...);
		return This;
	}

	/** Templated accessor for retrieving the shape as a concrete derived type. */
	template<typename Derived>
	FORCEINLINE Derived As() const
	{
		checkf(GetType() == Derived::StaticType, TEXT("Invalid cast from %d to expected type %d"), (int32)GetType(), (int32)Derived::StaticType);
		return Shape.Get<Derived>();
	}

	/** Templated accessor for retrieving the shape as a concrete derived type. */
	template<typename Derived>
	FORCEINLINE Derived& As()
	{
		checkf(GetType() == Derived::StaticType, TEXT("Invalid cast from %d to expected type %d"), (int32)GetType(), (int32)Derived::StaticType);
		return Shape.GetMutable<Derived>();
	}

	/**
	 * Templated accessor for retrieving the shape as a concrete derived type.
	 * Returns nullptr if type doesn't match.
	 */
	template<typename Derived>
	const Derived* TryAs() const
	{
		return GetType() == Derived::StaticType ? Shape.GetPtr<Derived>() : nullptr;
	}

	/**
	 * Templated accessor for retrieving the shape as a concrete derived type.
	 * Returns nullptr if type doesn't match.
	 */
	template<typename Derived>
	Derived* TryAs()
	{
		return GetType() == Derived::StaticType ? Shape.GetMutablePtr<Derived>() : nullptr;
	}

	/** Returns true if the contained shape instance is valid. */
	FORCEINLINE bool IsValid() const { return Shape.IsValid(); }

	/** Returns the underlying shape type. */
	FORCEINLINE EKzShapeType GetType() const
	{
		return IsValid() ? Shape.Get().GetType() : EKzShapeType::Sphere;
	}

	/** Returns true if this shape has zero extent (e.g. radius or half-size is zero). */
	FORCEINLINE bool IsZeroExtent() const { return IsValid() ? Shape.Get().IsZeroExtent() : true; }

	/** Computes the world-space axis-aligned bounding box (AABB) for this shape. */
	FORCEINLINE FBox GetAABB(const FTransform& Transform) const;

	/** Computes the world-space axis-aligned bounding box (AABB) for this shape. */
	FORCEINLINE FBox GetAABB(const FVector& Position, const FQuat& Orientation) const;

	/** Returns the closest point on (or inside) this shape to a given world-space point. */
	FORCEINLINE FVector GetClosestPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const;

	/** Checks whether a world-space point lies inside (or on the surface of) this shape. */
	FORCEINLINE bool ShapeIntersectsPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const;

	/** Tests intersection against a sphere. */
	FORCEINLINE bool ShapeIntersectsSphere(const FVector& Position, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius) const;

	/** Returns an engine-level FCollisionShape representing this Kz shape. */
	FORCEINLINE FCollisionShape ToCollisionShape(float Inflation = 0.0f) const
	{
		return IsValid() ? Shape.Get().ToCollisionShape(Inflation) : FCollisionShape::MakeBox(FVector::ZeroVector);
	}
};

#define SHAPE_OPERATORS(ShapeType) \
	FORCEINLINE ShapeType operator+(float Inflation, const ShapeType& Shape) \
	{ \
		return Shape.operator+(Inflation); \
	} \
	FORCEINLINE ShapeType operator+(const FVector& Inflation, const ShapeType& Shape) \
	{ \
		return Shape.operator+(Inflation); \
	} \
	FORCEINLINE ShapeType operator*(float Scale, const ShapeType& Shape) \
	{ \
		return Shape.operator*(Scale); \
	} \
	FORCEINLINE ShapeType operator*(const FVector& Scale, const ShapeType& Shape) \
	{ \
		return Shape.operator*(Scale); \
	}

SHAPE_OPERATORS(FKzSphere);
SHAPE_OPERATORS(FKzBox);
SHAPE_OPERATORS(FKzCapsule);
SHAPE_OPERATORS(FKzCylinder);
SHAPE_OPERATORS(FKzShapeInstance);

#undef SHAPE_OPERATORS
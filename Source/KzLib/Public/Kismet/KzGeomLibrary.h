// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KzGeomLibrary.generated.h"

struct FKzHitResult;
struct FKzShapeInstance;

UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzGeomLibrary"))
class KZLIB_API UKzGeomLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// === Ray intersection functions ===

	/**
	 * Performs a ray–shape intersection test.
	 * Returns true if the ray hits the shape within MaxDistance, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool RayIntersectsShape(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDirection, float MaxDistance, const FKzShapeInstance& Shape, const FVector ShapePosition, const FRotator ShapeRotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a ray–sphere intersection test.
	 * Returns true if the ray hits the sphere within MaxDistance, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool RayIntersectsSphere(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDirection, float MaxDistance, const FVector Center, float Radius, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a line segment–sphere intersection test.
	 * Returns true if the segment between Start and End intersects the sphere, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsSphere(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a ray–oriented-box intersection test.
	 * Returns true if the ray hits the box within MaxDistance, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool RayIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDirection, float MaxDistance, const FVector Center, const FVector HalfSize, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a line segment–oriented-box intersection test.
	 * Returns true if the segment between Start and End intersects the box, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, const FVector HalfSize, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a ray–capsule intersection test.
	 * Returns true if the ray hits the capsule within MaxDistance, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool RayIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDirection, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a line segment–capsule intersection test.
	 * Returns true if the segment between Start and End intersects the capsule, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a ray–cylinder intersection test.
	 * Returns true if the ray hits the cylinder within MaxDistance, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool RayIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDirection, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Performs a line segment–cylinder intersection test.
	 * Returns true if the segment between Start and End intersects the cylinder, filling OutHit with impact data.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	// === Shape ===

	/** Draw a debug shape */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawDebugShape(const UObject* WorldContextObject, const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, FLinearColor ShapeColor, float Duration = 0.f, float Thickness = 0.f);

	/**
	 * Returns the bounds of a shape in world space.
	 * @param Position			Center of the shape in world space.
	 * @param Rotation	Rotation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Shape Bounds"))
	static FBox GetShapeBounds(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape);

	/**
	 * Returns the closest point on (or inside) a shape to a given world-space point.
	 * @param Position			Center of the shape in world space.
	 * @param Rotation	Rotation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return The closest point on or inside the shape, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Shape"))
	static FVector ClosestPointOnShape(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) a shape.
	 * @param Position			Center of the shape in world space.
	 * @param Rotation	Rotation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return True if the point lies inside or on the shape; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Shape Intersects Point"))
	static bool ShapeIntersectsPoint(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, const FVector Point);

	// === Sphere ===

	/** Static utility function to make a sphere */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeSphere(const float Radius);

	/**
	 * Returns the bounds of a sphere in world space.
	 * @param Center Center of the sphere.
	 * @param Radius Radius of the sphere.
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FBox GetSphereBounds(const FVector Center, float Radius);

	/**
	 * Returns the closest point on (or inside) a sphere to a given world-space point.
	 * @param Center	Center of the sphere in world space.
	 * @param Radius				Radius of the sphere.
	 * @param Point					World-space point to test.
	 * @return The closest point on or inside the sphere, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry")
	static FVector ClosestPointOnSphere(const FVector Center, float Radius, FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) a sphere.
	 * @param Center	Center of the sphere in world space.
	 * @param Radius				Radius of the sphere.
	 * @param Point					World-space point to test.
	 * @return True if the point lies inside or on the sphere; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry")
	static bool SphereIntersectsPoint(const FVector Center, float Radius, const FVector Point);

	// === Box ===

	/** Static utility function to make a box */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeBox(const FVector HalfSize);

	/**
	 * Returns the bounds of an oriented box in world space.
	 * @param Center			Center of the box in world space.
	 * @param HalfSize		Half extents of the box along its local X, Y, Z axes.
	 * @param Rotation	Rotation of the box (defines its local axes).
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Box Bounds"))
	static FBox GetBoxBounds(const FVector Center, const FVector HalfSize, const FRotator Rotation);

	/**
	 * Returns the closest point on (or inside) an oriented box to a given world-space point.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Rotation	Rotation of the box (defines its local axes).
	 * @param Point						World-space point to test.
	 * @return The closest point on or inside the box, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Box"))
	static FVector ClosestPointOnBox(const FVector Center, const FVector HalfSize, const FRotator Rotation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented box.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Rotation	Rotation of the box (defines its local axes).
	 * @param Point						World-space point to test.
	 * @return True if the point lies inside or on the box; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Box Intersects Point"))
	static bool BoxIntersectsPoint(const FVector Center, const FVector HalfSize, const FRotator Rotation, const FVector Point);

	// === Capsule ===

	/** Static utility function to make a capsule */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeCapsule(const float Radius, const float HalfHeight);

	/**
	 * Returns the axis-aligned bounding box (Bounds) of an oriented capsule in world space.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Rotation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space Bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Capsule Bounds"))
	static FBox GetCapsuleBounds(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation);

	/**
	 * Returns the closest point on (or inside) an oriented capsule to a given world-space point.
	 * The capsule is defined by its center, half-height (from center to endcap center), radius, and orientation.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Rotation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the capsule, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Capsule"))
	static FVector ClosestPointOnCapsule(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented capsule.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Rotation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the capsule; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Capsule Intersects Point"))
	static bool CapsuleIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point);

	// === Cylinder ===

	/** Static utility function to make a cylinder */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeCylinder(const float Radius, const float HalfHeight);

	/**
	 * Returns the bounds of an oriented cylinder in world space.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Rotation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Cylinder Bounds"))
	static FBox GetCylinderBounds(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation);

	/**
	 * Returns the closest point on (or inside) an oriented cylinder to a given world-space point.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Rotation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the cylinder, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Cylinder"))
	static FVector ClosestPointOnCylinder(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented cylinder.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Rotation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the cylinder; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Cylinder Intersects Point"))
	static bool CylinderIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, FVector Point);

	// === Geometry ===

	/**
	 * Generates points distributed evenly on a sphere using the Fibonacci Lattice algorithm.
	 * @param NumSamples The number of points to generate.
	 * @param Radius     The radius of the sphere.
	 * @param Transform  World transform to apply.
	 * @return           Array containing the generated points.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (AutoCreateRefTerm = "Transform", AdvancedDisplay = "Transform"))
	static TArray<FVector> GetFibonacciSpherePoints(int32 NumSamples = 32, float Radius = 50.0f, const FTransform& Transform = FTransform());
};
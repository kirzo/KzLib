// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KzGeomLibrary.generated.h"

struct FKzShapeInstance;

UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzGeomLibrary"))
class KZLIB_API UKzGeomLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// === Spatial Queries / Shapes ===

	/**
	 * Tests intersection between a ray and a sphere.
	 * @param RayOrigin			Starting point of the ray in world space.
	 * @param RayDir				Normalized direction of the ray.
	 * @param RayLength			Length of the ray. Negative or zero means infinite length.
	 * @param SphereCenter	Center of the sphere.
	 * @param SphereRadius	Radius of the sphere.
	 * @param bStartInside	Set to true if the ray starts inside the sphere.
	 * @param Distance			Distance from RayOrigin to the intersection point (if any).
	 * @param HitPoint			Point of impact in world space (RayOrigin if inside).
	 * @return True if the ray intersects the sphere.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (Keywords = "raycast"))
	static bool RayIntersectsSphere(const FVector RayOrigin, const FVector RayDir, float RayLength, const FVector SphereCenter, float SphereRadius, bool& bStartInside, float& Distance, FVector& HitPoint);

	/**
	 * Tests intersection between a ray and an axis-aligned bounding box.
	 * @param RayOrigin				Origin of the ray.
	 * @param RayDir					Normalized direction of the ray.
	 * @param RayLength				Maximum distance along the ray (<=0 means infinite).
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half-size (extents) of the box along each axis.
	 * @param Orientation	Orientation of the box.
	 * @param bStartInside		Set to true if the ray starts inside the box.
	 * @param Distance				Distance from RayOrigin to the entry point (0 if inside).
	 * @param HitPoint				Point of impact in world space (RayOrigin if inside).
	 * @return True if the ray intersects the box.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (Keywords = "raycast"))
	static bool RayIntersectsBox(const FVector RayOrigin, const FVector RayDir, float RayLength, const FVector Center, const FVector HalfSize, const FQuat Orientation, bool& bStartInside, float& Distance, FVector& HitPoint);

	/**
	 * Tests intersection between a line and a sphere.
	 * @param Start					Start of line segment.
	 * @param End						End of line segment.
	 * @param SphereCenter	Center of the sphere.
	 * @param SphereRadius	Radius of the sphere.
	 * @param bStartInside	Set to true if the line starts inside the sphere.
	 * @param Distance			Distance from LineStart to the intersection point (if any).
	 * @param HitPoint			Point of impact in world space (LineStart if inside).
	 * @return True if the line intersects the sphere.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsSphere(const UObject* WorldContextObject, const FVector Start, const FVector End, const FVector SphereCenter, float SphereRadius, bool& bStartInside, float& Distance, FVector& HitPoint, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	 * Tests intersection between a line and an axis-aligned bounding box.
	 * @param Start						Start of line segment.
	 * @param End							End of line segment.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half-size (extents) of the box along each axis.
	 * @param Orientation	Orientation of the box.
	 * @param bStartInside		Set to true if the line starts inside the box.
	 * @param Distance				Distance from LineStart to the entry point (0 if inside).
	 * @param HitPoint				Point of impact in world space (LineStart if inside).
	 * @return True if the line intersects the box.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool LineIntersectsBox(const UObject* WorldContextObject, const FVector Start, const FVector End, const FVector Center, const FVector HalfSize, const FRotator Orientation, bool& bStartInside, float& Distance, FVector& HitPoint, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/** Draw a debug shape */
	UFUNCTION(BlueprintCallable, Category = "Rendering|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawDebugShape(const UObject* WorldContextObject, const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, FLinearColor ShapeColor, float Duration = 0.f, float Thickness = 0.f);

	/**
	 * Returns the axis-aligned bounding box (AABB) of a shape in world space.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @return FBox representing the world-space AABB.
	 */
	static FBox GetShapeAABB(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape);

	/**
	 * Returns the bounds of a shape in world space.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Shape Bounds"))
	static FBox K2_GetShapeAABB(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape);

	/**
	 * Returns the closest point on (or inside) a shape to a given world-space point.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return The closest point on or inside the shape, in world space.
	 */
	static FVector ClosestPointOnShape(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point);

	/**
	 * Returns the closest point on (or inside) a shape to a given world-space point.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return The closest point on or inside the shape, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Shape"))
	static FVector K2_ClosestPointOnShape(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) a shape.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return True if the point lies inside or on the shape; false otherwise.
	 */
	static bool ShapeIntersectsPoint(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) a shape.
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param Point							World-space point to test.
	 * @return True if the point lies inside or on the shape; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Shape Intersects Point"))
	static bool K2_ShapeIntersectsPoint(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point);

	/**
	 * Tests intersection between a sphere and a shape.
	 *
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param SphereCenter			Center of the sphere in world space.
	 * @param SphereRadius			Radius of the sphere.
	 * @return True if the shape and sphere intersect, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry")
	static bool ShapeIntersectsSphere(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& SphereCenter, float SphereRadius);

	/**
	 * Tests intersection between a sphere and a shape.
	 *
	 * @param Position			Center of the shape in world space.
	 * @param Orientation	Orientation of the shape in world space.
	 * @param Shape							Shape instance.
	 * @param SphereCenter			Center of the sphere in world space.
	 * @param SphereRadius			Radius of the sphere.
	 * @return True if the shape and sphere intersect, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Shape Intersects Sphere"))
	static bool K2_ShapeIntersectsSphere(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector SphereCenter, float SphereRadius);

	/** Static utility function to make a sphere */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeSphere(const float Radius);

	/**
	 * Returns the bounds of a sphere in world space.
	 * @param Center Center of the sphere.
	 * @param Radius Radius of the sphere.
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Sphere Bounds"))
	static FBox GetSphereAABB(const FVector Center, float Radius);

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

	/**
	 * Tests intersection between a sphere and a sphere.
	 *
	 * @param CenterA	Center of the first sphere in world space.
	 * @param RadiusA	Radius of the first sphere.
	 * @param CenterB	Center of the second sphere in world space.
	 * @param RadiusB	Radius of the second sphere.
	 * @return	True if the spheres intersect, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry")
	static bool SphereIntersectsSphere(const FVector CenterA, float RadiusA, const FVector CenterB, float RadiusB);

	/** Static utility function to make a box */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeBox(const FVector HalfSize);

	/**
	 * Returns the axis-aligned bounding box (AABB) of an oriented box in world space.
	 * @param Center			Center of the box in world space.
	 * @param HalfSize		Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @return FBox representing the world-space AABB.
	 */
	static FBox GetBoxAABB(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation);

	/**
	 * Returns the bounds of an oriented box in world space.
	 * @param Center			Center of the box in world space.
	 * @param HalfSize		Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Box Bounds"))
	static FBox K2_GetBoxAABB(const FVector Center, const FVector HalfSize, const FRotator Orientation);

	/**
	 * Returns the closest point on (or inside) an oriented box to a given world-space point.
	 * @param Center			Center of the box in world space.
	 * @param HalfSize		Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the box, in world space.
	 */
	static FVector ClosestPointOnBox(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point);

	/**
	 * Returns the closest point on (or inside) an oriented box to a given world-space point.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param Point						World-space point to test.
	 * @return The closest point on or inside the box, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Box"))
	static FVector K2_ClosestPointOnBox(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented box.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param Point						World-space point to test.
	 * @return True if the point lies inside or on the box; false otherwise.
	 */
	static bool BoxIntersectsPoint(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented box.
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param Point						World-space point to test.
	 * @return True if the point lies inside or on the box; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Box Intersects Point"))
	static bool K2_BoxIntersectsPoint(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point);

	/**
	 * Tests intersection between a sphere and an box.
	 *
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param SphereCenter		Center of the sphere in world space.
	 * @param SphereRadius		Radius of the sphere.
	 * @return True if the sphere and box intersect, false otherwise.
	 */
	static bool BoxIntersectsSphere(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius);

	/**
	 * Tests intersection between a sphere and an box.
	 *
	 * @param Center				Center of the box in world space.
	 * @param HalfSize			Half extents of the box along its local X, Y, Z axes.
	 * @param Orientation	Rotation of the box (defines its local axes).
	 * @param SphereCenter		Center of the sphere in world space.
	 * @param SphereRadius		Radius of the sphere.
	 * @return True if the sphere and box intersect, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Box Intersects Sphere"))
	static bool K2_BoxIntersectsSphere(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector SphereCenter, float SphereRadius);

	/** Static utility function to make a capsule */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeCapsule(const float Radius, const float HalfHeight);

	/**
	 * Returns the axis-aligned bounding box (AABB) of an oriented capsule in world space.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space AABB.
	 */
	static FBox GetCapsuleAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation);

	/**
	 * Returns the axis-aligned bounding box (AABB) of an oriented capsule in world space.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space AABB.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Capsule Bounds"))
	static FBox K2_GetCapsuleAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation);

	/**
	 * Returns the closest point on (or inside) an oriented capsule to a given world-space point.
	 * The capsule is defined by its center, half-height (from center to endcap center), radius, and orientation.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the capsule, in world space.
	 */
	static FVector ClosestPointOnCapsule(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point);

	/**
	 * Returns the closest point on (or inside) an oriented capsule to a given world-space point.
	 * The capsule is defined by its center, half-height (from center to endcap center), radius, and orientation.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the capsule, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Capsule"))
	static FVector K2_ClosestPointOnCapsule(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented capsule.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the capsule; false otherwise.
	 */
	static bool CapsuleIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented capsule.
	 * @param Center			Center of the capsule in world space.
	 * @param Radius			Radius of the capsule.
	 * @param HalfHeight	Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation	Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the capsule; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Capsule Intersects Point"))
	static bool K2_CapsuleIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point);

	/**
	 * Checks if a sphere intersects a capsule.
	 * @param Center				Center of the capsule in world space.
	 * @param Radius				Radius of the capsule.
	 * @param HalfHeight		Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation		Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param SphereCenter	Center of the sphere in world space.
	 * @param SphereRadius	Radius of the sphere.
	 * @return True if the sphere and capsule intersect; false otherwise.
	 */
	static bool CapsuleIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius);

	/**
	 * Checks if a sphere intersects a capsule.
	 * @param Center				Center of the capsule in world space.
	 * @param Radius				Radius of the capsule.
	 * @param HalfHeight		Distance from the capsule center to the center of either hemisphere cap (must be >= Radius).
	 * @param Orientation		Rotation of the capsule (its local +Z axis defines its height axis).
	 * @param SphereCenter	Center of the sphere in world space.
	 * @param SphereRadius	Radius of the sphere.
	 * @return True if the sphere and capsule intersect; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Capsule Intersects Sphere"))
	static bool K2_CapsuleIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius);

	/** Static utility function to make a cylinder */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry")
	static FKzShapeInstance MakeCylinder(const float Radius, const float HalfHeight);

	/**
	 * Returns the axis-aligned bounding box (AABB) of an oriented cylinder in world space.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space AABB.
	 */
	static FBox GetCylinderAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation);

	/**
	 * Returns the bounds of an oriented cylinder in world space.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @return FBox representing the world-space bounds.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|Geometry", meta = (DisplayName = "Get Cylinder Bounds"))
	static FBox K2_GetCylinderAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation);

	/**
	 * Returns the closest point on (or inside) an oriented cylinder to a given world-space point.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the cylinder, in world space.
	 */
	static FVector ClosestPointOnCylinder(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point);

	/**
	 * Returns the closest point on (or inside) an oriented cylinder to a given world-space point.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return The closest point on or inside the cylinder, in world space.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Closest Point on Cylinder"))
	static FVector K2_ClosestPointOnCylinder(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented cylinder.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the cylinder; false otherwise.
	 */
	static bool CylinderIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point);

	/**
	 * Checks whether a world-space point lies inside (or on the surface of) an oriented cylinder.
	 * @param Center			Center of the cylinder in world space.
	 * @param Radius			Radius of the cylinder.
	 * @param HalfHeight	Distance from the cylinder center to the top.
	 * @param Orientation	Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param Point				World-space point to test.
	 * @return True if the point lies inside or on the cylinder; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Cylinder Intersects Point"))
	static bool K2_CylinderIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, FVector Point);

	/**
	 * Checks if a sphere intersects a cylinder.
	 * @param Center				Center of the cylinder in world space.
	 * @param Radius				Radius of the cylinder.
	 * @param HalfHeight		Distance from the cylinder center to the top.
	 * @param Orientation		Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param SphereCenter	Center of the sphere in world space.
	 * @param SphereRadius	Radius of the sphere.
	 * @return True if the sphere and cylinder intersect; false otherwise.
	 */
	static bool CylinderIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius);

	/**
	 * Checks if a sphere intersects a cylinder.
	 * @param Center				Center of the cylinder in world space.
	 * @param Radius				Radius of the cylinder.
	 * @param HalfHeight		Distance from the cylinder center to the top.
	 * @param Orientation		Rotation of the cylinder (its local +Z axis defines its height axis).
	 * @param SphereCenter	Center of the sphere in world space.
	 * @param SphereRadius	Radius of the sphere.
	 * @return True if the sphere and cylinder intersect; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Geometry", meta = (DisplayName = "Cylinder Intersects Sphere"))
	static bool K2_CylinderIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius);
};
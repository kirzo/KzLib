// Copyright 2025 kirzo

#include "Kismet/KzGeomLibrary.h"
#include "Collision/KzHitResult.h"
#include "Collision/KzRaycast.h"
#include "Geometry/KzShapeInstance.h"
#include "Geometry/Shapes/CommonShapes.h"

#include "KismetTraceUtils.h"

#include "KzDrawDebugHelpers.h"

bool UKzGeomLibrary::RayIntersectsSphere(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, float Radius, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Sphere(OutHit, Center, Radius, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugSphere(World, Center, Radius, 12, bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsSphere(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsSphere(WorldContextObject, OutHit, Start, Dir, Len, Center, Radius, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

bool UKzGeomLibrary::RayIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Box(OutHit, Center, Orientation.Quaternion(), HalfSize, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugBox(World, Center, HalfSize, Orientation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsBox(WorldContextObject, OutHit, Start, Dir, Len, Center, HalfSize, Orientation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

bool UKzGeomLibrary::RayIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Capsule(OutHit, Center, Orientation.Quaternion(), Radius, HalfHeight, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugCapsule(World, Center, HalfHeight, Radius, Orientation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsCapsule(WorldContextObject, OutHit, Start, Dir, Len, Center, Radius, HalfHeight, Orientation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

bool UKzGeomLibrary::RayIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Cylinder(OutHit, Center, Orientation.Quaternion(), Radius, HalfHeight, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugCapsule(World, Center, HalfHeight, Radius, Orientation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsCylinder(WorldContextObject, OutHit, Start, Dir, Len, Center, Radius, HalfHeight, Orientation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}



void UKzGeomLibrary::DrawDebugShape(const UObject* WorldContextObject, const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, FLinearColor Color, float LifeTime, float Thickness)
{
#if ENABLE_DRAW_DEBUG
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		::DrawDebugShape(World, Position, Orientation.Quaternion(), Shape, Color.ToFColor(true), false, LifeTime, SDPG_World, Thickness);
	}
#endif
}

FBox UKzGeomLibrary::GetShapeAABB(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape)
{
	return Shape.GetBoundingBox(Position, Orientation);
}

FBox UKzGeomLibrary::K2_GetShapeAABB(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape)
{
	return GetShapeAABB(Position, Orientation.Quaternion(), Shape);
}

FVector UKzGeomLibrary::ClosestPointOnShape(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point)
{
	return Shape.GetClosestPoint(Position, Orientation, Point);
}

FVector UKzGeomLibrary::K2_ClosestPointOnShape(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point)
{
	return ClosestPointOnShape(Position, Orientation.Quaternion(), Shape, Point);
}

bool UKzGeomLibrary::ShapeIntersectsPoint(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& Point)
{
	return Shape.IntersectsPoint(Position, Orientation, Point);
}

bool UKzGeomLibrary::K2_ShapeIntersectsPoint(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector Point)
{
	return ShapeIntersectsPoint(Position, Orientation.Quaternion(), Shape, Point);
}

bool UKzGeomLibrary::ShapeIntersectsSphere(const FVector& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, const FVector& SphereCenter, float SphereRadius)
{
	return Shape.IntersectsSphere(Position, Orientation, SphereCenter, SphereRadius);
}

bool UKzGeomLibrary::K2_ShapeIntersectsSphere(const FVector Position, const FRotator Orientation, const FKzShapeInstance& Shape, const FVector SphereCenter, float SphereRadius)
{
	return ShapeIntersectsSphere(Position, Orientation.Quaternion(), Shape, SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeSphere(const float Radius)
{
	return FKzShapeInstance::Make<FKzSphere>(Radius);
}

FBox UKzGeomLibrary::GetSphereAABB(const FVector Center, float Radius)
{
	return FKzSphere(Radius).GetBoundingBox(Center, FQuat::Identity);
}

FVector UKzGeomLibrary::ClosestPointOnSphere(const FVector Center, float Radius, FVector Point)
{
	return FKzSphere(Radius).GetClosestPoint(Center, FQuat::Identity, Point);
}

bool UKzGeomLibrary::SphereIntersectsPoint(const FVector Center, float Radius, const FVector Point)
{
	return FKzSphere(Radius).IntersectsPoint(Center, FQuat::Identity, Point);
}

bool UKzGeomLibrary::SphereIntersectsSphere(const FVector CenterA, float RadiusA, const FVector CenterB, float RadiusB)
{
	return FKzSphere(RadiusA).IntersectsSphere(CenterA, FQuat::Identity, CenterB, RadiusB);
}

FKzShapeInstance UKzGeomLibrary::MakeBox(const FVector HalfSize)
{
	return FKzShapeInstance::Make<FKzBox>(HalfSize);
}

FBox UKzGeomLibrary::GetBoxAABB(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation)
{
	return FKzBox(HalfSize).GetBoundingBox(Center, Orientation);
}

FBox UKzGeomLibrary::K2_GetBoxAABB(const FVector Center, const FVector HalfSize, const FRotator Orientation)
{
	return GetBoxAABB(Center, HalfSize, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnBox(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point)
{
	return FKzBox(HalfSize).GetClosestPoint(Center, FQuat::Identity, Point);
}

FVector UKzGeomLibrary::K2_ClosestPointOnBox(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnBox(Center, HalfSize, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::BoxIntersectsPoint(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& Point)
{
	return FKzBox(HalfSize).IntersectsPoint(Center, Orientation, Point);
}

bool UKzGeomLibrary::K2_BoxIntersectsPoint(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector Point)
{
	return BoxIntersectsPoint(Center, HalfSize, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::BoxIntersectsSphere(const FVector& Center, const FVector& HalfSize, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	return FKzBox(HalfSize).IntersectsSphere(Center, Orientation, SphereCenter, SphereRadius);
}

bool UKzGeomLibrary::K2_BoxIntersectsSphere(const FVector Center, const FVector HalfSize, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return BoxIntersectsSphere(Center, HalfSize, Orientation.Quaternion(), SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeCapsule(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCapsuleAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation)
{
	return FKzCapsule(Radius, HalfHeight).GetBoundingBox(Center, Orientation);
}

FBox UKzGeomLibrary::K2_GetCapsuleAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation)
{
	return GetCapsuleAABB(Center, Radius, HalfHeight, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnCapsule(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	return FKzCapsule(Radius, HalfHeight).GetClosestPoint(Center, FQuat::Identity, Point);
}

FVector UKzGeomLibrary::K2_ClosestPointOnCapsule(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnCapsule(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CapsuleIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	return FKzCapsule(Radius, HalfHeight).IntersectsPoint(Center, Orientation, Point);
}

bool UKzGeomLibrary::K2_CapsuleIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return CapsuleIntersectsPoint(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CapsuleIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	return FKzCapsule(Radius, HalfHeight).IntersectsSphere(Center, Orientation, SphereCenter, SphereRadius);
}

bool UKzGeomLibrary::K2_CapsuleIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return CapsuleIntersectsSphere(Center, Radius, HalfHeight, Orientation.Quaternion(), SphereCenter, SphereRadius);
}

FKzShapeInstance UKzGeomLibrary::MakeCylinder(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCylinder>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCylinderAABB(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation)
{
	return FKzCylinder(Radius, HalfHeight).GetBoundingBox(Center, Orientation);
}

FBox UKzGeomLibrary::K2_GetCylinderAABB(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation)
{
	return GetCylinderAABB(Center, Radius, HalfHeight, Orientation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnCylinder(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	return FKzCylinder(Radius, HalfHeight).GetClosestPoint(Center, FQuat::Identity, Point);
}

FVector UKzGeomLibrary::K2_ClosestPointOnCylinder(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector Point)
{
	return ClosestPointOnCylinder(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CylinderIntersectsPoint(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& Point)
{
	return FKzCylinder(Radius, HalfHeight).IntersectsPoint(Center, Orientation, Point);
}

bool UKzGeomLibrary::K2_CylinderIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, FVector Point)
{
	return CylinderIntersectsPoint(Center, Radius, HalfHeight, Orientation.Quaternion(), Point);
}

bool UKzGeomLibrary::CylinderIntersectsSphere(const FVector& Center, float Radius, float HalfHeight, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius)
{
	return FKzCylinder(Radius, HalfHeight).IntersectsSphere(Center, Orientation, SphereCenter, SphereRadius);
}

bool UKzGeomLibrary::K2_CylinderIntersectsSphere(const FVector Center, float Radius, float HalfHeight, const FRotator Orientation, const FVector SphereCenter, float SphereRadius)
{
	return CylinderIntersectsSphere(Center, Radius, HalfHeight, Orientation.Quaternion(), SphereCenter, SphereRadius);
}
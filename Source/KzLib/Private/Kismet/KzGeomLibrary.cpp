// Copyright 2025 kirzo

#include "Kismet/KzGeomLibrary.h"

#include "Math/Geometry/KzGeometry.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Math/Geometry/Shapes/CommonShapes.h"
#include "Math/Geometry/KzSampling.h"

#include "Collision/KzHitResult.h"
#include "Collision/KzRaycast.h"
#include "Collision/KzGJK.h"

#include "KismetTraceUtils.h"

#include "KzDrawDebugHelpers.h"

// === Ray intersection functions ===

bool UKzGeomLibrary::RayIntersectsShape(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FKzShapeInstance& Shape, const FVector Position, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::GJK::Raycast(OutHit, RayStart, RayDir, MaxDistance, Shape , Position, Rotation.Quaternion());

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	::DrawDebugShape(World, Position, Rotation.Quaternion(), Shape, bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

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

bool UKzGeomLibrary::RayIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, const FVector HalfSize, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Box(OutHit, Center, Rotation.Quaternion(), HalfSize, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugBox(World, Center, HalfSize, Rotation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsBox(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, const FVector HalfSize, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsBox(WorldContextObject, OutHit, Start, Dir, Len, Center, HalfSize, Rotation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

bool UKzGeomLibrary::RayIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Capsule(OutHit, Center, Rotation.Quaternion(), Radius, HalfHeight, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsCapsule(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsCapsule(WorldContextObject, OutHit, Start, Dir, Len, Center, Radius, HalfHeight, Rotation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

bool UKzGeomLibrary::RayIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector RayStart, const FVector RayDir, float MaxDistance, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const bool bHit = Kz::Raycast::Cylinder(OutHit, Center, Rotation.Quaternion(), Radius, HalfHeight, RayStart, RayDir, MaxDistance);

#if ENABLE_DRAW_DEBUG
	bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
	float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	DrawDebugLineTraceSingle(World, OutHit.TraceStart, OutHit.TraceEnd, DrawDebugType, bHit, OutHit.ToHitResult(), TraceColor, TraceHitColor, DrawTime);
	DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation.Quaternion(), bHit ? TraceHitColor.ToFColor(true) : TraceColor.ToFColor(true), bPersistent, LifeTime);
#endif

	return bHit;
}

bool UKzGeomLibrary::LineIntersectsCylinder(const UObject* WorldContextObject, FKzHitResult& OutHit, const FVector Start, const FVector End, const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	FVector Dir; float Len;
	(End - Start).ToDirectionAndLength(Dir, Len);

	return RayIntersectsCylinder(WorldContextObject, OutHit, Start, Dir, Len, Center, Radius, HalfHeight, Rotation, DrawDebugType, TraceColor, TraceHitColor, DrawTime);
}

// === Shape ===

void UKzGeomLibrary::DrawDebugShape(const UObject* WorldContextObject, const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, FLinearColor Color, float LifeTime, float Thickness)
{
#if ENABLE_DRAW_DEBUG
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		::DrawDebugShape(World, Position, Rotation.Quaternion(), Shape, Color.ToFColor(true), false, LifeTime, SDPG_World, Thickness);
	}
#endif
}

FBox UKzGeomLibrary::GetShapeBounds(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape)
{
	return Shape.GetBoundingBox(Position, Rotation.Quaternion());
}

FVector UKzGeomLibrary::ClosestPointOnShape(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, const FVector Point)
{
	return Shape.GetClosestPoint(Position, Rotation.Quaternion(), Point);
}

bool UKzGeomLibrary::ShapeIntersectsPoint(const FVector Position, const FRotator Rotation, const FKzShapeInstance& Shape, const FVector Point)
{
	return Shape.IntersectsPoint(Position, Rotation.Quaternion(), Point);
}

// === Sphere ===

FKzShapeInstance UKzGeomLibrary::MakeSphere(const float Radius)
{
	return FKzShapeInstance::Make<FKzSphere>(Radius);
}

FBox UKzGeomLibrary::GetSphereBounds(const FVector Center, float Radius)
{
	return Kz::Geom::SphereBounds(Center, Radius);
}

FVector UKzGeomLibrary::ClosestPointOnSphere(const FVector Center, float Radius, FVector Point)
{
	return Kz::Geom::ClosestPointOnSphere(Center, Radius, Point);
}

bool UKzGeomLibrary::SphereIntersectsPoint(const FVector Center, float Radius, const FVector Point)
{
	return Kz::Geom::SphereIntersectsPoint(Center, Radius, Point);
}

// === Box ===

FKzShapeInstance UKzGeomLibrary::MakeBox(const FVector HalfSize)
{
	return FKzShapeInstance::Make<FKzBox>(HalfSize);
}

FBox UKzGeomLibrary::GetBoxBounds(const FVector Center, const FVector HalfSize, const FRotator Rotation)
{
	return Kz::Geom::BoxBounds(Center, Rotation.Quaternion(), HalfSize);
}

FVector UKzGeomLibrary::ClosestPointOnBox(const FVector Center, const FVector HalfSize, const FRotator Rotation, const FVector Point)
{
	return Kz::Geom::ClosestPointOnBox(Center, Rotation.Quaternion(), HalfSize, Point);
}

bool UKzGeomLibrary::BoxIntersectsPoint(const FVector Center, const FVector HalfSize, const FRotator Rotation, const FVector Point)
{
	return Kz::Geom::BoxIntersectsPoint(Center, Rotation.Quaternion(), HalfSize, Point);
}

// === Capsule ===

FKzShapeInstance UKzGeomLibrary::MakeCapsule(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCapsuleBounds(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation)
{
	return Kz::Geom::CapsuleBounds(Center, Rotation.Quaternion(), Radius, HalfHeight);
}

FVector UKzGeomLibrary::ClosestPointOnCapsule(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point)
{
	return Kz::Geom::ClosestPointOnCapsule(Center, Rotation.Quaternion(), Radius, HalfHeight, Point);
}

bool UKzGeomLibrary::CapsuleIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point)
{
	return Kz::Geom::CapsuleIntersectsPoint(Center, Rotation.Quaternion(), Radius, HalfHeight, Point);
}

// === Cylinder ===

FKzShapeInstance UKzGeomLibrary::MakeCylinder(const float Radius, const float HalfHeight)
{
	return FKzShapeInstance::Make<FKzCylinder>(Radius, HalfHeight);
}

FBox UKzGeomLibrary::GetCylinderBounds(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation)
{
	return Kz::Geom::CylinderBounds(Center, Rotation.Quaternion(), Radius, HalfHeight);
}

FVector UKzGeomLibrary::ClosestPointOnCylinder(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, const FVector Point)
{
	return Kz::Geom::ClosestPointOnCylinder(Center, Rotation.Quaternion(), Radius, HalfHeight, Point);
}

bool UKzGeomLibrary::CylinderIntersectsPoint(const FVector Center, float Radius, float HalfHeight, const FRotator Rotation, FVector Point)
{
	return Kz::Geom::CylinderIntersectsPoint(Center, Rotation.Quaternion(), Radius, HalfHeight, Point);
}

TArray<FVector> UKzGeomLibrary::GetFibonacciSpherePoints(int32 NumSamples, float Radius, const FTransform& Transform)
{
	TArray<FVector> Points;
	Kz::Geom::Sample::FibonacciSphere(NumSamples, Radius, Transform, Points);
	return Points;
}
// Copyright 2025 kirzo

#include "Geometry/KzShape.h"
#include "Kismet/KzGeomLibrary.h"

FCollisionShape FKzSphere::ToCollisionShape(float Inflation) const
{
	return FCollisionShape::MakeSphere(Radius + Inflation);
}

FCollisionShape FKzBox::ToCollisionShape(float Inflation) const
{
	return FCollisionShape::MakeBox(HalfSize + Inflation);
}

FCollisionShape FKzCapsule::ToCollisionShape(float Inflation) const
{
	return FCollisionShape::MakeCapsule(Radius + Inflation, HalfHeight + Inflation);
}

FCollisionShape FKzCylinder::ToCollisionShape(float Inflation) const
{
	return FCollisionShape::MakeBox(FVector(Radius, Radius, HalfHeight) + Inflation);
}

FBox FKzShapeInstance::GetAABB(const FTransform& Transform) const
{
	return UKzGeomLibrary::GetShapeAABB(Transform.GetLocation(), Transform.GetRotation(), *this);
}

FBox FKzShapeInstance::GetAABB(const FVector& Position, const FQuat& Orientation) const
{
	return UKzGeomLibrary::GetShapeAABB(Position, Orientation, *this);
}

FVector FKzShapeInstance::GetClosestPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const
{
	return UKzGeomLibrary::ClosestPointOnShape(Position, Orientation, *this, Point);
}

bool FKzShapeInstance::ShapeIntersectsPoint(const FVector& Position, const FQuat& Orientation, const FVector& Point) const
{
	return UKzGeomLibrary::ShapeIntersectsPoint(Position, Orientation, *this, Point);
}

bool FKzShapeInstance::ShapeIntersectsSphere(const FVector& Position, const FQuat& Orientation, const FVector& SphereCenter, float SphereRadius) const
{
	return UKzGeomLibrary::ShapeIntersectsSphere(Position, Orientation, *this, SphereCenter, SphereRadius);
}
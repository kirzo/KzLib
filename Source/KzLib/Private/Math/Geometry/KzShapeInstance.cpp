// Copyright 2025 kirzo

#pragma once

#include "Math/Geometry/KzShapeInstance.h"
#include "Math/Geometry/Shapes/KzBox.h"
#include "Math/Geometry/Shapes/KzSphere.h"
#include "Math/Geometry/Shapes/KzCapsule.h"

FKzShapeInstance::FKzShapeInstance()
{
	Shape.InitializeAs<FKzSphere>();
}

FKzShapeInstance FKzShapeInstance::MakeFromCollisionShape(const FCollisionShape& CollisionShape)
{
	if (CollisionShape.IsBox())
	{
		return FKzShapeInstance::Make<FKzBox>(CollisionShape.GetExtent());
	}
	else if (CollisionShape.IsSphere())
	{
		return FKzShapeInstance::Make<FKzSphere>(CollisionShape.GetSphereRadius());
	}
	else if (CollisionShape.IsCapsule())
	{
		return FKzShapeInstance::Make<FKzCapsule>(CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleHalfHeight());
	}

	return FKzShapeInstance::Make<FKzSphere>(1.0f);
}
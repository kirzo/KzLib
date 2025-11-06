// Copyright 2025 kirzo

#include "KzDrawDebugHelpers.h"
#include "Geometry/KzShape.h"

#if ENABLE_DRAW_DEBUG

extern ENGINE_API float GServerDrawDebugColorTintStrength;
extern ENGINE_API FLinearColor GServerDrawDebugColorTint;

#if WITH_EDITOR

FColor AdjustColorForServer(const FColor InColor)
{
	if (GServerDrawDebugColorTintStrength > 0.0f)
	{
		return FMath::Lerp(FLinearColor::FromSRGBColor(InColor), GServerDrawDebugColorTint, GServerDrawDebugColorTintStrength).ToFColor(/*bSRGB=*/ true);
	}
	else
	{
		return InColor;
	}
}

bool CanDrawServerDebugInContext(const FWorldContext& WorldContext)
{
	return
		(WorldContext.WorldType == EWorldType::PIE) &&
		(WorldContext.World() != nullptr) &&
		(WorldContext.World()->GetNetMode() == NM_Client) &&
		(WorldContext.GameViewport != nullptr) &&
		(WorldContext.GameViewport->EngineShowFlags.ServerDrawDebug);
}

#define UE_DRAW_SERVER_DEBUG_ON_EACH_CLIENT(FunctionName, ...) \
		if (GIsEditor) \
		{ \
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts()) \
			{ \
				if (CanDrawServerDebugInContext(WorldContext)) \
				{ \
					FunctionName(WorldContext.World(), __VA_ARGS__); \
				} \
			} \
		}

#else

#define UE_DRAW_SERVER_DEBUG_ON_EACH_CLIENT(FunctionName, ...)

#endif

void DrawDebugShape(const UWorld* InWorld, FVector const& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	// no debug line drawing on dedicated server
	if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer)
	{
		switch (Shape.GetType())
		{
			case EKzShapeType::Sphere:
			{
				DrawDebugSphere(InWorld, Position, Shape.As<FKzSphere>().Radius, 12, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
				break;
			}
			case EKzShapeType::Box:
			{
				DrawDebugBox(InWorld, Position, Shape.As<FKzBox>().HalfSize, Orientation, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
				break;
			}
			case EKzShapeType::Capsule:
			{
				const FKzCapsule Capsule = Shape.As<FKzCapsule>();
				DrawDebugCapsule(InWorld, Position, Capsule.HalfHeight, Capsule.Radius, Orientation, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
				break;
			}
			case EKzShapeType::Cylinder:
			{
				const FKzCylinder Cylinder = Shape.As<FKzCylinder>();
				const FVector UpVector = Orientation.GetUpVector();
				DrawDebugCylinder(InWorld, Position - UpVector * Cylinder.HalfHeight, Position + UpVector * Cylinder.HalfHeight, Cylinder.Radius, 12, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
				break;
			}
			default: break;
		}
	}
	else
	{
		UE_DRAW_SERVER_DEBUG_ON_EACH_CLIENT(DrawDebugShape, Position, Orientation, Shape, AdjustColorForServer(Color), bPersistentLines, LifeTime, DepthPriority, Thickness);
	}
}

#endif // ENABLE_DRAW_DEBUG
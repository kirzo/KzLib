// Copyright 2026 kirzo

#include "KzDrawDebugHelpers.h"
#include "Math/Geometry/KzShapeInstance.h"
#include "Engine/Engine.h"

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
		if (Shape.IsValid())
		{
			Shape.As<FKzShape>().DrawDebug(InWorld, Position, Orientation, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		}
	}
	else
	{
		UE_DRAW_SERVER_DEBUG_ON_EACH_CLIENT(DrawDebugShape, Position, Orientation, Shape, AdjustColorForServer(Color), bPersistentLines, LifeTime, DepthPriority, Thickness);
	}
}

#endif // ENABLE_DRAW_DEBUG
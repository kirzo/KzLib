// Copyright 2025 kirzo

#pragma once

#include "DrawDebugHelpers.h"

struct FKzShapeInstance;

#ifndef ENABLE_DRAW_DEBUG
#define ENABLE_DRAW_DEBUG  UE_ENABLE_DEBUG_DRAWING
#endif

#if ENABLE_DRAW_DEBUG

KZLIB_API void DrawDebugShape(const UWorld* InWorld, FVector const& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f);

#elif !defined(SHIPPING_DRAW_DEBUG_ERROR) || !SHIPPING_DRAW_DEBUG_ERROR

// Empty versions of above functions
FORCEINLINE void DrawDebugShape(const UWorld* InWorld, FVector const& Position, const FQuat& Orientation, const FKzShapeInstance& Shape, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) {}

#else

#endif // ENABLE_DRAW_DEBUG
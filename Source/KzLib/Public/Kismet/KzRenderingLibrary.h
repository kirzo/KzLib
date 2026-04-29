// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzRenderingLibrary.generated.h"

class AActor;
class USceneCaptureComponent2D;

/** Rendering-related Blueprint utilities (texture streaming, scene capture, etc.). */
UCLASS(meta = (ScriptName = "KzRenderingLibrary"))
class KZLIB_API UKzRenderingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// === Streaming ===

	/**
	 * Registers an additional view origin with the texture streaming system, so it
	 * considers this location when deciding which mips to load. Useful for
	 * SceneCaptureComponent2D placed far from any player camera, which otherwise
	 * renders with the lowest mip and looks blurry.
	 *
	 * Typically called every tick (Duration = 0) or periodically with a matching Duration.
	 *
	 * @param ViewOrigin        World-space location of the virtual viewpoint.
	 * @param ScreenWidth       Width in pixels of the target being rendered (e.g. RenderTarget->SizeX).
	 * @param FOV               Horizontal field of view, in degrees.
	 * @param BoostFactor       LOD boost (1 = normal, higher = more detail).
	 * @param bOverrideLocation If true, forces the streaming system to ignore all other regular view locations.
	 * @param Duration          Seconds the streaming system should keep this view active. 0 = next tick only.
	 * @param ActorToBoost      Optional actor whose textures get prioritized.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Rendering|Streaming")
	static void AddStreamingViewOrigin(const FVector& ViewOrigin, float ScreenWidth, float FOV, float BoostFactor = 1.0f, bool bOverrideLocation = false, float Duration = 0.0f, AActor* ActorToBoost = nullptr);

	/**
	 * Convenience overload: registers a streaming view derived from a SceneCaptureComponent2D's
	 * world location, FOV, and render target size. Call every tick (or with Duration matching
	 * your call interval) on captures placed far from the player to avoid blurry textures.
	 *
	 * @param SceneCapture  The capture component to use as the virtual viewpoint. Must have a valid TextureTarget.
	 * @param BoostFactor   LOD boost (1 = normal, higher = more detail).
	 * @param Duration      Seconds the streaming system should keep this view active. 0 = next tick only.
	 * @param ActorToBoost  Optional actor whose textures get prioritized.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Rendering|Streaming", meta = (DefaultToSelf = "SceneCapture"))
	static void AddStreamingViewFromSceneCapture(USceneCaptureComponent2D* SceneCapture, float BoostFactor = 1.0f, float Duration = 0.0f, AActor* ActorToBoost = nullptr);
};
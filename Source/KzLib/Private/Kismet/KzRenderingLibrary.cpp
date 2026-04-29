// Copyright 2026 kirzo

#include "Kismet/KzRenderingLibrary.h"

#include "Components/SceneCaptureComponent2D.h"
#include "ContentStreaming.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"

void UKzRenderingLibrary::AddStreamingViewOrigin(const FVector& ViewOrigin, float ScreenWidth, float FOV, float BoostFactor, bool bOverrideLocation, float Duration, AActor* ActorToBoost)
{
	if (ScreenWidth <= 0.0f || FOV <= 0.0f || FOV >= 180.0f)
	{
		return;
	}

	const float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	const float TanHalf = FMath::Tan(HalfFOVRad);
	if (TanHalf <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	IStreamingManager::Get().AddViewInformation(ViewOrigin, ScreenWidth, ScreenWidth / TanHalf, BoostFactor, bOverrideLocation, Duration, ActorToBoost);
}

void UKzRenderingLibrary::AddStreamingViewFromSceneCapture(USceneCaptureComponent2D* SceneCapture, float BoostFactor, float Duration, AActor* ActorToBoost)
{
	if (!SceneCapture)
	{
		return;
	}

	UTextureRenderTarget2D* RT = SceneCapture->TextureTarget;
	if (!RT || RT->SizeX <= 0)
	{
		return;
	}

	AddStreamingViewOrigin(SceneCapture->GetComponentLocation(), static_cast<float>(RT->SizeX), SceneCapture->FOVAngle, BoostFactor, false, Duration, ActorToBoost);
}
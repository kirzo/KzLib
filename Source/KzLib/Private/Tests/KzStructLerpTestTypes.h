// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzStructLerpTestTypes.generated.h"

USTRUCT()
struct FKzStructLerpTestInner
{
	GENERATED_BODY()

	UPROPERTY()
	double Value = 0.0;
};

/** Every kind of member Kz::LerpStruct treats differently. */
USTRUCT()
struct FKzStructLerpTestStruct
{
	GENERATED_BODY()

	FKzStructLerpTestStruct()
		: bOverride_Gated(false)
		, bOverride_Ungated(false)
	{
	}

	UPROPERTY()
	uint8 bOverride_Gated : 1;

	UPROPERTY()
	uint8 bOverride_Ungated : 1;

	UPROPERTY()
	float Float = 0.0f;

	UPROPERTY()
	int32 Int = 0;

	UPROPERTY()
	FLinearColor Linear = FLinearColor::Black;

	UPROPERTY()
	FColor Color = FColor::Black;

	UPROPERTY()
	FRotator Rotator = FRotator::ZeroRotator;

	UPROPERTY()
	bool bBool = false;

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FKzStructLerpTestInner Inner;

	UPROPERTY()
	float Gated = 0.0f;

	UPROPERTY()
	float Ungated = 0.0f;
};
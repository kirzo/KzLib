// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionKzSaturationGamma.generated.h"

/** Scales a colour's HSV saturation, then raises it to a power. */
UCLASS(collapsecategories, hidecategories = Object, meta = (DisplayName = "Saturation Gamma"))
class UMaterialExpressionKzSaturationGamma : public UMaterialExpression
{
	GENERATED_BODY()

public:
	UMaterialExpressionKzSaturationGamma();

	UPROPERTY()
	FExpressionInput Input;

	/** Multiplies the HSV saturation: 0 turns the colour grey at its brightest channel, 1 leaves it. */
	UPROPERTY(EditAnywhere, Category = "Saturation Gamma")
	float Saturation = 0.5f;

	/** The power the desaturated colour is raised to. */
	UPROPERTY(EditAnywhere, Category = "Saturation Gamma")
	float Gamma = 2.2f;

#if WITH_EDITOR
	/** ponytail: legacy translator only, add Build() once a project turns on r.Material.Translator.EnableNew. */
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
};
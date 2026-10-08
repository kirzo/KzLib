// Copyright 2026 kirzo

#include "Materials/MaterialExpressionKzSaturationGamma.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "MaterialExpressionKzSaturationGamma"

UMaterialExpressionKzSaturationGamma::UMaterialExpressionKzSaturationGamma()
{
#if WITH_EDITORONLY_DATA
	MenuCategories.Add(LOCTEXT("Color", "Color"));
#endif
}

#if WITH_EDITOR
int32 UMaterialExpressionKzSaturationGamma::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	if (!Input.GetTracedInput().Expression)
	{
		return Compiler->Errorf(TEXT("Missing Saturation Gamma input"));
	}

	const int32 Color = Compiler->ForceCast(Input.Compile(Compiler), MCT_Float3, MFCF_ExactMatch | MFCF_ReplicateValue);
	const int32 Brightest = Compiler->Max(Compiler->ComponentMask(Color, true, false, false, false), Compiler->Max(Compiler->ComponentMask(Color, false, true, false, false), Compiler->ComponentMask(Color, false, false, true, false)));

	// Scaling the HSV saturation moves every channel toward the brightest one in proportion, so no round trip through hue is needed.
	return Compiler->Power(Compiler->Lerp(Brightest, Color, Compiler->Constant(Saturation)), Compiler->Constant(Gamma));
}

void UMaterialExpressionKzSaturationGamma::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Saturation Gamma"));
}
#endif

#undef LOCTEXT_NAMESPACE
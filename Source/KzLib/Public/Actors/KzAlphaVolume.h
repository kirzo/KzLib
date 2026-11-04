// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Curves/CurveFloat.h"
#include "KzAlphaVolume.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UKzShapeComponent;

/**
 * A volume that gives an alpha for a location. Inside, it is 1, or when progressive, the position along Axis from the entry face (0) to the exit face (1).
 * Past the exit face, within the volume's extrusion along Axis, a progressive volume keeps 1, so what it brings in stays once crossed;
 * a non-progressive one only if it keeps past exit. Anywhere else outside, the alpha is 0.
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZLIB_API AKzAlphaVolume : public AActor
{
	GENERATED_BODY()

public:
	AKzAlphaVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alpha Volume")
	TObjectPtr<UKzShapeComponent> ShapeComponent;

	/** If false, the alpha is 1 anywhere inside. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume")
	bool bProgressive = true;

	/** Keeps the alpha at 1 after leaving through the exit face, instead of dropping it on leaving. Progressive volumes always do. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume", meta = (EditCondition = "!bProgressive"))
	bool bKeepPastExit = false;

	/** World direction the alpha grows along, from the entry face to the exit face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume", meta = (EditCondition = "bProgressive || bKeepPastExit"))
	FVector Axis = FVector::ForwardVector;

	/** Remaps the progressive alpha. Empty means linear. */
	UPROPERTY(EditAnywhere, Category = "Alpha Volume", meta = (EditCondition = "bProgressive", XAxisName = "Position", YAxisName = "Alpha"))
	FRuntimeFloatCurve Curve;

	UFUNCTION(BlueprintPure, Category = "Alpha Volume")
	float GetAlpha(const FVector& Location) const;

	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;

	/** Runs along Axis from the entry face to the exit face. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> AxisArrow = nullptr;
#endif

private:
	bool UsesAxis() const { return bProgressive || bKeepPastExit; }

	/** Normalized Axis and where the entry and exit faces are along it, relative to the shape's centre. False if Axis is zero. */
	bool GetAxisExtent(FVector& OutDirection, double& OutMin, double& OutMax) const;
};

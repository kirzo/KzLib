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
 * A non-directional volume that keeps past exit has no exit face: whoever reads it keeps what it brought however it leaves (KeepsAfterLeaving).
 */
UCLASS(Blueprintable, HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD))
class KZLIB_API AKzAlphaVolume : public AActor
{
	GENERATED_BODY()

public:
	AKzAlphaVolume();

	/** If false, the alpha is 1 anywhere inside. A progressive volume grows along Axis and keeps 1 past the exit face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume")
	bool bProgressive = false;

	/** Remaps the progressive alpha. Empty means linear. */
	UPROPERTY(EditAnywhere, Category = "Alpha Volume", meta = (EditCondition = "bProgressive", EditConditionHides, XAxisName = "Position", YAxisName = "Alpha"))
	FRuntimeFloatCurve Curve;

	/** Keeps the alpha at 1 after leaving, instead of dropping it: past the exit face if directional, however it is left if not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume", meta = (EditCondition = "!bProgressive", EditConditionHides))
	bool bKeepPastExit = true;

	/** Leaving through the exit face along Axis keeps the alpha, and leaving where it came in drops it. Without it, any way out keeps it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume", meta = (EditCondition = "!bProgressive && bKeepPastExit", EditConditionHides))
	bool bDirectional = true;

	/** World direction the alpha grows along, from the entry face to the exit face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alpha Volume", meta = (EditCondition = "bProgressive || (bKeepPastExit && bDirectional)", EditConditionHides))
	FVector Axis = FVector::ForwardVector;

	/** After the settings, so they show above the shape's categories. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alpha Volume")
	TObjectPtr<UKzShapeComponent> ShapeComponent;

	UFUNCTION(BlueprintPure, Category = "Alpha Volume")
	float GetAlpha(const FVector& Location) const;

	/** Whoever was inside keeps what the volume brought after its alpha drops to 0, until a jump (start, respawn) re-reads it. */
	bool KeepsAfterLeaving() const { return !bProgressive && bKeepPastExit && !bDirectional; }

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
	bool UsesAxis() const { return bProgressive || (bKeepPastExit && bDirectional); }

	/** Normalized Axis and where the entry and exit faces are along it, relative to the shape's centre. False if Axis is zero. */
	bool GetAxisExtent(FVector& OutDirection, double& OutMin, double& OutMax) const;
};

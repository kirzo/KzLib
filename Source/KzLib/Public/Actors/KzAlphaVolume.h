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
 * That is what a jump (start, respawn) reads. Whoever walks out through the exit face keeps what the volume brought while past it,
 * wherever it goes from there, such as up a slope over the extrusion; a non-directional volume that keeps past exit keeps it however it is left (Keeps).
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
	bool bKeepPastExit = false;

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

	/** Whether Location is inside the shape, which is how a walking reader brings the volume in: reaching past the exit some other way doesn't. */
	UFUNCTION(BlueprintPure, Category = "Alpha Volume")
	bool Contains(const FVector& Location) const;

	/**
	 * Whether whoever brought the volume in keeps it now that its alpha at Location is 0: past the exit plane along Axis, or anywhere for a
	 * non-directional volume that keeps past exit. Only for walking out: after a jump, the alpha alone decides.
	 */
	bool Keeps(const FVector& Location) const;

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

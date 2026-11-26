// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzStandIn.generated.h"

class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * A character stand-in for previews in the editor: a cylinder of its size, or a skeletal mesh in its reference pose.
 * Moved in the editor, it settles on the ground. It shows in every editor view, the camera previews included, has no collision
 * and destroys itself when the game starts.
 */
UCLASS(HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD, Rendering))
class KZLIB_API AKzStandIn : public AActor
{
	GENERATED_BODY()

public:
	AKzStandIn();

	/** Shown instead of the cylinder, in its reference pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (ClampMin = "1", Units = "Centimeters"))
	float Height = 180.0f;

	/** The cylinder's, and how wide the ground check is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (ClampMin = "1", Units = "Centimeters"))
	float Radius = 35.0f;

	/**
	 * Moved in the editor, it settles on the ground between Snap Above and Snap Below its feet: of the walkable surfaces there, the one
	 * nearest its height, so it follows slopes and steps up and down and doesn't climb onto a bridge it passes under. With none, it stays.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In")
	bool bSnapToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (EditCondition = "bSnapToGround", EditConditionHides, ClampMin = "0", Units = "Centimeters"))
	float SnapAbove = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (EditCondition = "bSnapToGround", EditConditionHides, ClampMin = "0", Units = "Centimeters"))
	float SnapBelow = 200.0f;

	/** What counts as ground: what blocks this channel. Pawns don't count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (EditCondition = "bSnapToGround", EditConditionHides))
	TEnumAsByte<ECollisionChannel> SnapChannel = ECC_Pawn;

	/** Settles on the ground under it, if there is any within reach. */
	UFUNCTION(CallInEditor, Category = "Stand-In")
	void SnapToGround();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category = "Stand-In")
	TObjectPtr<UStaticMeshComponent> Cylinder;

	UPROPERTY(VisibleAnywhere, Category = "Stand-In")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/** What the cylinder's red instance is made from. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> CylinderMaterial;

	/** The mesh or the cylinder, at the stand-in's size and in red. */
	void UpdateLook();

	/** Where its feet go on the ground within reach, nearest its current height. */
	bool FindGround(FVector& OutLocation) const;
};
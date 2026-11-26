// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzStandIn.generated.h"

class UCameraComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UStreamableRenderAsset;
struct FMinimalViewInfo;

/**
 * A character stand-in for previews in the editor: a cylinder of its size, or a mesh, skeletal in its reference pose or static.
 * Moved in the editor, it settles on the ground. It shows in every editor view, the camera previews included, has no collision
 * and destroys itself when the game starts. Its target location is half its height up, where a character's capsule centre is.
 * Selected, its preview shows whatever view is set on it, such as the camera that would show it.
 */
UCLASS(HideCategories = (Input, Collision, Physics, LOD, Cooking, Replication, HLOD, Rendering))
class KZLIB_API AKzStandIn : public AActor
{
	GENERATED_BODY()

public:
	AKzStandIn();

	/** Shown instead of the cylinder: a skeletal mesh, in its reference pose, or a static mesh, such as a pose made with Make Static Mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (AllowedClasses = "/Script/Engine.SkeletalMesh,/Script/Engine.StaticMesh"))
	TObjectPtr<UStreamableRenderAsset> Mesh;

	/** Turns the mesh about its feet, for one that doesn't face the actor's forward: -90 for a character modelled facing Y. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stand-In", meta = (EditCondition = "Mesh != nullptr", EditConditionHides, UIMin = "-180", UIMax = "180", Units = "Degrees"))
	float YawOffset = 0.0f;

	/** The character's capsule height: whoever follows the stand-in follows the middle of it. */
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

	/** What its preview shows: whoever has the view to preview sets it every frame. */
	void SetPreviewView(const FMinimalViewInfo& View);

	virtual FVector GetTargetLocation(AActor* RequestedBy = nullptr) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void UnregisterAllComponents(bool bForReregister = false) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	// Not editable: the stand-in sets them from its own properties, which would undo any change made on them.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Cylinder;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** Gives the stand-in an editor preview, which shows the view set on it. */
	UPROPERTY()
	TObjectPtr<UCameraComponent> PreviewCamera;

	/** What the cylinder's red instance is made from. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> CylinderMaterial;

	/** The mesh or the cylinder, at the stand-in's size and in red. */
	void UpdateLook();

	/** Where its feet go on the ground within reach, nearest its current height. */
	bool FindGround(FVector& OutLocation) const;
};
// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
#include "Geometry/KzShapeInstance.h"
#include "KzShapeComponent.generated.h"

/** A component that defines a renderable geometric shape using the KzLib shape system. */
UCLASS(ClassGroup = "Rendering", hidecategories = (Object, Collision, LOD, Lighting, TextureStreaming, Physics, Navigation, HLOD, PathTracing, Tags, Cooking, Mobile, RayTracing, AssetUserData), ShowCategories = (Activation), editinlinenew, meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzShapeComponent : public UShapeComponent
{
	GENERATED_BODY()

public:
	/** Shape definition (sphere, box, capsule, etc.) */
	UPROPERTY(EditAnywhere, Category = Shape, meta = (ShowOnlyInnerProperties))
	FKzShapeInstance Shape;

	/** If true, solid geometry will be rendered in addition to wireframe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, export, Category = Shape, AdvancedDisplay)
	bool bDrawSolid = false;

public:
	UKzShapeComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool IsZeroExtent() const override;
	virtual struct FCollisionShape GetCollisionShape(float Inflation = 0.0f) const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const override;
	//~ End USceneComponent Interface

	//~ Begin UShapeComponent Interface
	virtual void UpdateBodySetup() override;
	//~ End UShapeComponent Interface
};
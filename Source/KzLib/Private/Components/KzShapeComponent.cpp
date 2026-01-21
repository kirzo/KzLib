// Copyright 2026 kirzo

#include "Components/KzShapeComponent.h"
#include "Math/Geometry/Shapes/CommonShapes.h"
#include "SceneView.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "Engine/Engine.h"

UKzShapeComponent::UKzShapeComponent()
{
	bAutoActivate = true;
	bCanEverAffectNavigation = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	Shape.InitializeAs<FKzSphere>();
	ShapeColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f).ToFColor(true);

	SetGenerateOverlapEvents(false);
}

#if WITH_EDITOR
void UKzShapeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKzShapeComponent, Shape) || PropertyName == GET_MEMBER_NAME_CHECKED(UKzShapeComponent, bDrawSolid))
	{
		MarkRenderStateDirty();
	}
}
#endif

FPrimitiveSceneProxy* UKzShapeComponent::CreateSceneProxy()
{
	class FKzShapeSceneProxy  final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		/** Initialization constructor. */
		FKzShapeSceneProxy (const UKzShapeComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
			, bDrawOnlyIfSelected(InComponent->bDrawOnlyIfSelected)
			, Shape(InComponent->Shape)
			, ShapeColor(InComponent->ShapeColor)
			, LineThickness(InComponent->LineThickness)
			, bDrawSolid(InComponent->bDrawSolid)
		{
			bWillEverBeLit = false;
		}

		// FPrimitiveSceneProxy interface.

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_ShapeSceneProxy_GetDynamicMeshElements);

			if (!Shape.IsValid())
			{
				return;
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					const FMatrix& LocalToWorld = GetLocalToWorld();

					// Taking into account the min and maximum drawing distance
					const float DistanceSqr = (View->ViewMatrices.GetViewOrigin() - LocalToWorld.GetOrigin()).SizeSquared();
					if (DistanceSqr < FMath::Square(GetMinDrawDistance()) || DistanceSqr > FMath::Square(GetMaxDrawDistance()))
					{
						continue;
					}

					const FLinearColor DrawColor = GetViewSelectionColor(ShapeColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());
					FKzShapeInstance ScaledShape = Shape * LocalToWorld.GetScaleVector();
					ScaledShape.As<FKzShape>().DrawSceneProxy(PDI, LocalToWorld, DrawColor, bDrawSolid, LineThickness, ViewIndex, Collector);
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			const bool bVisibleForSelection = !bDrawOnlyIfSelected || IsSelected();
			const bool bVisibleForShowFlags = true;

			// Should we draw this because collision drawing is enabled, and we have collision
			const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();

			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = (IsShown(View) && bVisibleForSelection && bVisibleForShowFlags) || bShowForCollision;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			Result.bSeparateTranslucency = true;
			Result.bNormalTranslucency = true;
			return Result;
		}

		virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }
		uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	private:
		const uint32				bDrawOnlyIfSelected : 1;
		FKzShapeInstance		Shape;
		const FColor				ShapeColor;
		const float					LineThickness;
		const uint32				bDrawSolid : 1;
	};

	return new FKzShapeSceneProxy (this);
}

bool UKzShapeComponent::IsZeroExtent() const
{
	return Shape.IsZeroExtent();
}

FCollisionShape UKzShapeComponent::GetCollisionShape(float Inflation) const
{
	return (Shape * GetComponentScale()).ToCollisionShape(Inflation);
}

FBoxSphereBounds UKzShapeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return Shape.GetBoundingBox(LocalToWorld);
}

void UKzShapeComponent::CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const
{
	const FVector BoundsHalfsize = Shape.GetBoundingBox(GetComponentTransform()).GetExtent();
	CylinderRadius = FMath::Sqrt(FMath::Square(BoundsHalfsize.X) + FMath::Square(BoundsHalfsize.Y));
	CylinderHalfHeight = BoundsHalfsize.Z;
}

void UKzShapeComponent::UpdateBodySetup()
{
}
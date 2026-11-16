// Copyright 2026 kirzo

#include "Components/KzShapeComponent.h"
#include "Math/Geometry/Shapes/CommonShapes.h"
#include "SceneView.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

	const FName MemberName = PropertyChangedEvent.GetMemberPropertyName();
	if (MemberName != GET_MEMBER_NAME_CHECKED(UKzShapeComponent, Shape) && MemberName != GET_MEMBER_NAME_CHECKED(UKzShapeComponent, bDrawSolid))
	{
		return;
	}

	MarkRenderStateDirty();

	// The details panel only propagates archetype edits that go through ImportText. The instanced struct picker
	// writes the picked shape straight into the raw memory of the edited object, so a Blueprint preview actor
	// keeps rendering the previous shape until the Blueprint is recompiled. Push the new one to it by hand.
	if (IsTemplate() && MemberName == GET_MEMBER_NAME_CHECKED(UKzShapeComponent, Shape))
	{
		TArray<UObject*> Instances;
		GetArchetypeInstances(Instances);

		for (UObject* Instance : Instances)
		{
			UKzShapeComponent* PreviewComponent = Cast<UKzShapeComponent>(Instance);
			const UWorld* PreviewWorld = PreviewComponent ? PreviewComponent->GetWorld() : nullptr;
			if (PreviewWorld && PreviewWorld->WorldType == EWorldType::EditorPreview)
			{
				PreviewComponent->Shape = Shape;
				PreviewComponent->MarkRenderStateDirty();
			}
		}
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
	if (!Shape.IsValid())
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.0);
	}

	return (Shape * LocalToWorld.GetScale3D()).GetBoundingBox(LocalToWorld);
}

void UKzShapeComponent::CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const
{
	const FVector BoundsHalfsize = CalcBounds(GetComponentTransform()).BoxExtent;
	CylinderRadius = FMath::Sqrt(FMath::Square(BoundsHalfsize.X) + FMath::Square(BoundsHalfsize.Y));
	CylinderHalfHeight = BoundsHalfsize.Z;
}

void UKzShapeComponent::UpdateBodySetup()
{
}
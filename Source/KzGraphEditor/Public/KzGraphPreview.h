// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class FAdvancedPreviewScene;
class FCanvas;
class FPrimitiveDrawInterface;
class HHitProxy;
class FSceneView;
class FViewport;
class SWidget;

/**
 * What a graph editor's viewport shows, for the kinds of graph that want one: their editor returns it from CreatePreview.
 * The base runs the scene, with its floor, light and the project's preview profile, the camera, the toolbar and the ticking.
 */
class KZGRAPHEDITOR_API FKzGraphPreview : public TSharedFromThis<FKzGraphPreview>
{
public:
	virtual ~FKzGraphPreview() = default;

	/** Fills the scene once the viewport has made it. */
	virtual void Initialize(FAdvancedPreviewScene& Scene) {}

	virtual void OnSelectionChanged(const TSet<UObject*>& Selection) {}
	virtual void OnGraphChanged() {}

	/** Runs every frame, once the scene's world has moved on, so poses read are this frame's. */
	virtual void Tick(float DeltaTime) {}

	/** Draws over the scene, in the world and on the screen. */
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) {}
	virtual void DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas) {}

	/** What the camera frames. */
	virtual FBox GetBounds() const { return FBox(ForceInit); }

	/** Which way the camera looks when the viewport opens; unset keeps the editor's default. */
	virtual TOptional<FRotator> GetViewRotation() const { return {}; }

	/** A panel under the viewport, such as a timeline. */
	virtual TSharedPtr<SWidget> MakeBottomPanel() { return nullptr; }

	/** A left click in the viewport, on the hit proxy drawn there, if any. True when the preview took it. */
	virtual bool HandleClick(HHitProxy* HitProxy) { return false; }

	/** Where the transform gizmo shows, on what the preview has selected to move; unset hides it. */
	virtual TOptional<FTransform> GetGizmoTransform() const { return {}; }

	/** The gizmo was dragged by Drag and turned by Rotation, both in world space. */
	virtual void MoveGizmo(const FVector& Drag, const FRotator& Rotation) {}

	/** A drag of the gizmo starts or ends, for one undoable change per drag. */
	virtual void BeginGizmoDrag() {}
	virtual void EndGizmoDrag() {}
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class FAdvancedPreviewScene;
class FCanvas;
class FPrimitiveDrawInterface;
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

	/** Runs every frame, before the scene's world moves on. */
	virtual void Tick(float DeltaTime) {}

	/** Draws over the scene, in the world and on the screen. */
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) {}
	virtual void DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas) {}

	/** What the camera frames. */
	virtual FBox GetBounds() const { return FBox(ForceInit); }

	/** A panel under the viewport, such as a timeline. */
	virtual TSharedPtr<SWidget> MakeBottomPanel() { return nullptr; }
};
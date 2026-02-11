// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/** Visualizer for KzActorGroup to draw lines and brackets around grouped actors */
class FKzActorGroupVisualizer : public FComponentVisualizer
{
public:
	//~ Begin FComponentVisualizer Interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	//~ End FComponentVisualizer Interface
};
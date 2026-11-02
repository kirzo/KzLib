// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FKzGraphPreview;

/** A graph editor's viewport: the usual toolbar and orbit camera over a preview scene whose contents the graph's preview decides. */
class SKzGraphViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SKzGraphViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FKzGraphPreview>& InPreview);

	/** Puts the camera on what the preview shows. */
	void Frame();

protected:
	virtual void BindCommands() override;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> BuildViewportToolbar() override;
	virtual void OnFocusViewportToSelection() override;

private:
	TSharedPtr<FKzGraphPreview> Preview;
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
};
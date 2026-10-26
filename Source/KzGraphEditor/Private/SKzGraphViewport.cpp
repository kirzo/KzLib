// Copyright 2026 kirzo

#include "SKzGraphViewport.h"
#include "AdvancedPreviewScene.h"
#include "AdvancedPreviewSceneMenus.h"
#include "EditorViewportClient.h"
#include "GameFramework/WorldSettings.h"
#include "KzGraphPreview.h"
#include "ToolMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"
#include "ViewportToolbar/UnrealEdViewportToolbarContext.h"

namespace
{
	// Orbits the preview and runs its world, which a preview scene does not do on its own.
	class FKzGraphViewportClient : public FEditorViewportClient
	{
	public:
		FKzGraphViewportClient(FAdvancedPreviewScene& InPreviewScene, const TSharedRef<SKzGraphViewport>& InViewport, const TSharedRef<FKzGraphPreview>& InPreview)
			: FEditorViewportClient(nullptr, &InPreviewScene, InViewport)
			, Preview(InPreview)
		{
			SetRealtime(true);
			ToggleOrbitCamera(true);
		}

		virtual void Tick(float DeltaSeconds) override
		{
			FEditorViewportClient::Tick(DeltaSeconds);
			PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
			Preview->Tick(DeltaSeconds);
		}

		virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override
		{
			FEditorViewportClient::Draw(View, PDI);
			Preview->Draw(View, PDI);
		}

		virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override
		{
			FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
			Preview->DrawCanvas(InViewport, View, Canvas);
		}

	private:
		TSharedRef<FKzGraphPreview> Preview;
	};
}

void SKzGraphViewport::Construct(const FArguments& InArgs, const TSharedRef<FKzGraphPreview>& InPreview)
{
	Preview = InPreview;
	PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());

	// Without this the game's floor of half a millisecond a frame keeps a paused preview creeping on.
	PreviewScene->GetWorld()->GetWorldSettings()->MinUndilatedFrameTime = 0.0f;

	SEditorViewport::Construct(SEditorViewport::FArguments());
	Preview->Initialize(*PreviewScene);
	Frame();
}

void SKzGraphViewport::Frame()
{
	const FBox Bounds = Preview->GetBounds();
	if (Bounds.IsValid && Client.IsValid())
	{
		Client->FocusViewportOnBox(Bounds);
	}
}

TSharedRef<FEditorViewportClient> SKzGraphViewport::MakeEditorViewportClient()
{
	const TSharedRef<FEditorViewportClient> ViewportClient = MakeShared<FKzGraphViewportClient>(*PreviewScene, SharedThis(this), Preview.ToSharedRef());

	// The profile's post processing and tone mapping reach the view as show flags, as in the engine's asset viewers.
	UE::AdvancedPreviewScene::BindDefaultOnSettingsChangedHandler(PreviewScene, ViewportClient);
	return ViewportClient;
}

TSharedPtr<SWidget> SKzGraphViewport::BuildViewportToolbar()
{
	const FName ToolbarName = TEXT("KzGraphEditor.ViewportToolbar");
	if (!UToolMenus::Get()->IsMenuRegistered(ToolbarName))
	{
		UToolMenu* Toolbar = UToolMenus::Get()->RegisterMenu(ToolbarName, NAME_None, EMultiBoxType::SlimHorizontalToolBar);
		Toolbar->StyleName = TEXT("ViewportToolbar");
		FToolMenuSection& Left = Toolbar->AddSection(TEXT("Left"));
		Left.AddEntry(UE::UnrealEd::CreateTransformsSubmenu());

		FToolMenuSection& Right = Toolbar->AddSection(TEXT("Right"));
		Right.Alignment = EToolMenuSectionAlign::Last;
		Right.AddEntry(UE::UnrealEd::CreateCameraSubmenu(UE::UnrealEd::FViewportCameraMenuOptions().ShowLensControls()));
		Right.AddEntry(UE::UnrealEd::CreateViewModesSubmenu());
		Right.AddEntry(UE::UnrealEd::CreateShowSubmenu(FNewToolMenuDelegate::CreateLambda([](UToolMenu* Menu) { UE::UnrealEd::AddDefaultShowFlags(Menu); })));
		Right.AddEntry(UE::UnrealEd::CreatePerformanceAndScalabilitySubmenu());
	}

	FToolMenuContext Context;
	Context.AppendCommandList(PreviewScene->GetCommandList());
	Context.AppendCommandList(GetCommandList());
	Context.AddObject(UE::UnrealEd::CreateViewportToolbarDefaultContext(SharedThis(this)));
	return UToolMenus::Get()->GenerateWidget(ToolbarName, Context);
}

void SKzGraphViewport::OnFocusViewportToSelection()
{
	Frame();
}
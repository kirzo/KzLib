// Copyright 2026 kirzo

#include "KzGraphEditor.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditAction.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "KzEdGraph.h"
#include "KzEdGraphNode.h"
#include "KzEdGraphSchema.h"
#include "KzGraphEditorUtils.h"
#include "KzGraph.h"
#include "KzGraphCompiler.h"
#include "KzGraphPreview.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SKzGraphFindResults.h"
#include "SKzGraphViewport.h"
#include "Framework/SlateDelegates.h"
#include "SNodePanel.h"
#include "Styling/AppStyle.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "UObject/Package.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SKzValidationPanel.h"
#include "Widgets/SNullWidget.h"

#define LOCTEXT_NAMESPACE "KzGraphEditor"

namespace
{
	const FName AppId(TEXT("KzGraphEditor"));
	const FName GraphTabId(TEXT("KzGraphEditor_Graph"));
	const FName DetailsTabId(TEXT("KzGraphEditor_Details"));
	const FName CompileResultsTabId(TEXT("KzGraphEditor_CompileResults"));
	const FName FindTabId(TEXT("KzGraphEditor_Find"));
	const FName ViewportTabId(TEXT("KzGraphEditor_Viewport"));

	const TCHAR* SettingsSection = TEXT("KzGraphEditor");
	const TCHAR* AutoCompileKey = TEXT("bAutoCompile");

	/** Holds the graph editor and sees its keys first, so the base can answer some its own way. */
	class SKzGraphKeyHost : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKzGraphKeyHost) {}
			SLATE_EVENT(FOnKeyDown, OnPreviewKeyDown)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			PreviewKeyDown = InArgs._OnPreviewKeyDown;
			ChildSlot
			[
				InArgs._Content.Widget
			];
		}

		virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			return PreviewKeyDown.IsBound() ? PreviewKeyDown.Execute(MyGeometry, InKeyEvent) : FReply::Unhandled();
		}

	private:
		FOnKeyDown PreviewKeyDown;
	};
}

FKzGraphEditorCommands::FKzGraphEditorCommands()
	: TCommands<FKzGraphEditorCommands>(TEXT("KzGraphEditor"), LOCTEXT("CommandsContext", "Graph Editor"), NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FKzGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND(Compile, "Compile", "Rebuilds the nodes from their code and compiles the graph, listing the problems found.", EUserInterfaceActionType::Button, FInputChord(EKeys::F7));
	UI_COMMAND(AutoCompile, "Auto Compile", "Compiles after every edit, once the mouse is let go. Applies to every graph, for you only.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Find, "Find", "Searches the graph's nodes, pins and comments.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
}

void FKzGraphEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, const TArray<UObject*>& Objects)
{
	for (UObject* Object : Objects)
	{
		if (UKzGraph* Asset = Cast<UKzGraph>(Object))
		{
			MakeShared<FKzGraphEditor>()->Initialize(Mode, Host, Asset);
		}
	}
}

FKzGraphEditor::~FKzGraphEditor()
{
	FTSTicker::GetCoreTicker().RemoveTicker(AutoCompileTicker);
	if (UKzEdGraph* EdGraph = GetEdGraph())
	{
		EdGraph->OnCompiled.RemoveAll(this);
		EdGraph->RemoveOnGraphChangedHandler(GraphChangedHandle);
	}
}

void FKzGraphEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UKzGraph* InGraph)
{
	Graph = InGraph;
	if (!InGraph->EdGraph)
	{
		UKzEdGraph::CreateFor(*InGraph);
	}
	UKzEdGraph* EdGraph = GetEdGraph();

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsArgs.bHideSelectionTip = true;
	DetailsView = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsArgs);
	DetailsView->SetObject(InGraph);

	FindResults = SNew(SKzGraphFindResults, EdGraph).OnResultActivated(this, &FKzGraphEditor::JumpToNode);
	CompileResults = SNew(SKzValidationPanel)
		.OnIssueActivated(this, &FKzGraphEditor::OnIssueActivated)
		.OnRunValidation(this, &FKzGraphEditor::CompileForResults);

	// A dry compile finds the problems again without touching the asset.
	FKzGraphData Data;
	TArray<FKzValidationIssue> Found;
	KzGraphCompiler::Compile(*EdGraph, *GetTransientPackage(), Data, Found);
	ShowIssues(Found);

	EdGraph->OnCompiled.AddSP(this, &FKzGraphEditor::OnCompiled);
	GraphChangedHandle = EdGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FKzGraphEditor::OnGraphChanged));
	AutoCompileTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FKzGraphEditor::TickAutoCompile));

	BindCommands();

	const TSharedRef<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP(this, &FKzGraphEditor::FillToolbar));
	AddToolbarExtender(ToolbarExtender);

	Preview = CreatePreview();
	if (Preview.IsValid())
	{
		const TSharedPtr<SWidget> BottomPanel = Preview->MakeBottomPanel();
		PreviewPanel = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SKzGraphViewport, Preview.ToSharedRef())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BottomPanel.IsValid() ? BottomPanel.ToSharedRef() : SNullWidget::NullWidget
			];
	}

	// Compile results and search open below the graph when asked for, as in Blueprints.
	const TSharedRef<FTabManager::FSplitter> GraphColumn = FTabManager::NewSplitter()
		->SetOrientation(Orient_Vertical)
		->SetSizeCoefficient(Preview.IsValid() ? 0.45f : 0.75f)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.8f)
			->AddTab(GraphTabId, ETabState::OpenedTab)
			->SetHideTabWell(true)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.2f)
			->AddTab(CompileResultsTabId, ETabState::ClosedTab)
			->AddTab(FindTabId, ETabState::ClosedTab)
		);

	const TSharedRef<FTabManager::FArea> Area = FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal);
	if (Preview.IsValid())
	{
		Area->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.3f)
			->AddTab(ViewportTabId, ETabState::OpenedTab)
		);
	}
	Area->Split(GraphColumn)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.25f)
			->AddTab(DetailsTabId, ETabState::OpenedTab)
		);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(Preview.IsValid() ? "KzGraphEditor_Preview_Layout_v1" : "KzGraphEditor_Layout_v1")->AddArea(Area);

	InitAssetEditor(Mode, Host, AppId, Layout, true, true, InGraph);
}

UKzEdGraph* FKzGraphEditor::GetEdGraph() const
{
	return Graph.IsValid() ? Cast<UKzEdGraph>(Graph->EdGraph) : nullptr;
}

void FKzGraphEditor::JumpToNode(UEdGraphNode* Node)
{
	TabManager->TryInvokeTab(GraphTabId);
	if (GraphEditor.IsValid() && Node)
	{
		GraphEditor->JumpToNode(Node, false, true);
	}
}

void FKzGraphEditor::BindCommands()
{
	const TSharedRef<FUICommandList>& Commands = GetToolkitCommands();
	const FGenericCommands& Generic = FGenericCommands::Get();
	const FGraphEditorCommandsImpl& GraphCommands = FGraphEditorCommands::Get();

	Commands->MapAction(Generic.Delete, FExecuteAction::CreateSP(this, &FKzGraphEditor::DeleteSelectedNodes, false), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanDeleteNodes));
	Commands->MapAction(GraphCommands.DeleteAndReconnectNodes, FExecuteAction::CreateSP(this, &FKzGraphEditor::DeleteSelectedNodes, true), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanDeleteNodes));
	Commands->MapAction(Generic.Copy, FExecuteAction::CreateSP(this, &FKzGraphEditor::CopySelectedNodes), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanCopyNodes));
	Commands->MapAction(Generic.Cut, FExecuteAction::CreateSP(this, &FKzGraphEditor::CutSelectedNodes), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanCutNodes));
	Commands->MapAction(Generic.Paste, FExecuteAction::CreateSP(this, &FKzGraphEditor::PasteNodes), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanPasteNodes));
	Commands->MapAction(Generic.Duplicate, FExecuteAction::CreateSP(this, &FKzGraphEditor::DuplicateNodes), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanCopyNodes));
	Commands->MapAction(Generic.Rename, FExecuteAction::CreateSP(this, &FKzGraphEditor::RenameSelectedNode), FCanExecuteAction::CreateSP(this, &FKzGraphEditor::CanRenameNode));
	Commands->MapAction(Generic.SelectAll, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->SelectAllNodes(); }));
	Commands->MapAction(GraphCommands.BreakNodeLinks, FExecuteAction::CreateSP(this, &FKzGraphEditor::BreakSelectedNodeLinks));
	Commands->MapAction(GraphCommands.ReconstructNodes, FExecuteAction::CreateSP(this, &FKzGraphEditor::ReconstructSelectedNodes));
	Commands->MapAction(GraphCommands.CreateComment, FExecuteAction::CreateSP(this, &FKzGraphEditor::CreateComment));

	Commands->MapAction(GraphCommands.AlignNodesTop, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignTop(); }));
	Commands->MapAction(GraphCommands.AlignNodesMiddle, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignMiddle(); }));
	Commands->MapAction(GraphCommands.AlignNodesBottom, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignBottom(); }));
	Commands->MapAction(GraphCommands.AlignNodesLeft, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignLeft(); }));
	Commands->MapAction(GraphCommands.AlignNodesCenter, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignCenter(); }));
	Commands->MapAction(GraphCommands.AlignNodesRight, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnAlignRight(); }));
	Commands->MapAction(GraphCommands.StraightenConnections, FExecuteAction::CreateSP(this, &FKzGraphEditor::StraightenConnections));
	Commands->MapAction(GraphCommands.DistributeNodesHorizontally, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnDistributeNodesH(); }));
	Commands->MapAction(GraphCommands.DistributeNodesVertically, FExecuteAction::CreateSPLambda(this, [this]() { GraphEditor->OnDistributeNodesV(); }));

	// The engine's graph editor binds these itself, ahead of the toolkit's: taken before it sees the key.
	PreviewCommands = MakeShared<FUICommandList>();
	PreviewCommands->MapAction(GraphCommands.StraightenConnections, FExecuteAction::CreateSP(this, &FKzGraphEditor::StraightenConnections));

	const FKzGraphEditorCommands& KzCommands = FKzGraphEditorCommands::Get();
	Commands->MapAction(KzCommands.Compile, FExecuteAction::CreateSP(this, &FKzGraphEditor::Compile));
	Commands->MapAction(KzCommands.AutoCompile, FExecuteAction::CreateSP(this, &FKzGraphEditor::ToggleAutoCompile), FCanExecuteAction(), FIsActionChecked::CreateSP(this, &FKzGraphEditor::IsAutoCompile));
	Commands->MapAction(KzCommands.Find, FExecuteAction::CreateSP(this, &FKzGraphEditor::FindInGraph));
}

void FKzGraphEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FKzGraphEditorCommands& Commands = FKzGraphEditorCommands::Get();

	ToolbarBuilder.BeginSection("Compile");
	ToolbarBuilder.AddToolBarButton(Commands.Compile, NAME_None, TAttribute<FText>(), TAttribute<FText>::CreateSP(this, &FKzGraphEditor::GetCompileTooltip), TAttribute<FSlateIcon>::CreateSP(this, &FKzGraphEditor::GetCompileIcon));
	ToolbarBuilder.AddComboButton(FUIAction(), FOnGetContent::CreateSP(this, &FKzGraphEditor::MakeCompileOptions), LOCTEXT("CompileOptions", "Compile Options"), LOCTEXT("CompileOptionsTooltip", "How the graph compiles."), FSlateIcon(), true);
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Find");
	ToolbarBuilder.AddToolBarButton(Commands.Find, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults"));
	ToolbarBuilder.EndSection();
}

FName FKzGraphEditor::GetToolkitFName() const
{
	return AppId;
}

FText FKzGraphEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Graph Editor");
}

FString FKzGraphEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Graph ").ToString();
}

FLinearColor FKzGraphEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FKzGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	const TSharedRef<FWorkspaceItem> Category = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "Graph Editor"));
	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FKzGraphEditor::SpawnGraphTab)).SetDisplayName(LOCTEXT("GraphTab", "Graph")).SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x")).SetGroup(Category);
	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FKzGraphEditor::SpawnDetailsTab)).SetDisplayName(LOCTEXT("DetailsTab", "Details")).SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")).SetGroup(Category);
	InTabManager->RegisterTabSpawner(CompileResultsTabId, FOnSpawnTab::CreateSP(this, &FKzGraphEditor::SpawnCompileResultsTab)).SetDisplayName(LOCTEXT("CompileResultsTab", "Compile Results")).SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.CompilerResults")).SetGroup(Category);
	InTabManager->RegisterTabSpawner(FindTabId, FOnSpawnTab::CreateSP(this, &FKzGraphEditor::SpawnFindTab)).SetDisplayName(LOCTEXT("FindTab", "Find Results")).SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults")).SetGroup(Category);
	if (Preview.IsValid())
	{
		InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FKzGraphEditor::SpawnViewportTab)).SetDisplayName(LOCTEXT("ViewportTab", "Viewport")).SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports")).SetGroup(Category);
	}
}

void FKzGraphEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(CompileResultsTabId);
	InTabManager->UnregisterTabSpawner(FindTabId);
	InTabManager->UnregisterTabSpawner(ViewportTabId);
}

void FKzGraphEditor::PostUndo(bool bSuccess)
{
	// The selection may point at nodes that are gone.
	if (GraphEditor.IsValid())
	{
		GraphEditor->ClearSelectionSet();
		GraphEditor->NotifyGraphChanged();
	}

	FSlateApplication::Get().DismissAllMenus();
}

void FKzGraphEditor::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

TSharedRef<SDockTab> FKzGraphEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FKzGraphEditor::OnSelectionChanged);
	Events.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FKzGraphEditor::OnNodeTitleCommitted);
	Events.OnSpawnNodeByShortcutAtLocation = SGraphEditor::FOnSpawnNodeByShortcutAtLocation::CreateSP(this, &FKzGraphEditor::OnSpawnNodeByShortcut);
	Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FKzGraphEditor::OnNodeDoubleClicked);

	FGraphAppearanceInfo Appearance;
	Appearance.CornerText = Graph.IsValid() ? Graph->GetClass()->GetDisplayNameText().ToUpper() : FText::GetEmpty();

	GraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(GetToolkitCommands())
		.GraphToEdit(GetEdGraph())
		.GraphEvents(Events)
		.Appearance(Appearance);

	return SNew(SDockTab).Label(LOCTEXT("GraphTab", "Graph"))
		[
			SNew(SKzGraphKeyHost)
			.OnPreviewKeyDown(this, &FKzGraphEditor::OnGraphPreviewKeyDown)
			[
				GraphEditor.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FKzGraphEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).Label(LOCTEXT("DetailsTab", "Details"))[DetailsView.ToSharedRef()];
}

TSharedRef<SDockTab> FKzGraphEditor::SpawnCompileResultsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).Label(LOCTEXT("CompileResultsTab", "Compile Results"))[CompileResults.ToSharedRef()];
}

TSharedRef<SDockTab> FKzGraphEditor::SpawnFindTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).Label(LOCTEXT("FindTab", "Find Results"))[FindResults.ToSharedRef()];
}

TSharedRef<SDockTab> FKzGraphEditor::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).Label(LOCTEXT("ViewportTab", "Viewport"))[PreviewPanel.ToSharedRef()];
}

void FKzGraphEditor::OnSelectionChanged(const TSet<UObject*>& Selection)
{
	if (Selection.IsEmpty())
	{
		DetailsView->SetObject(Graph.Get());
	}
	else
	{
		DetailsView->SetObjects(Selection.Array());
	}

	if (Preview.IsValid())
	{
		Preview->OnSelectionChanged(Selection);
	}
}

void FKzGraphEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	if (const UKzEdGraphNode* KzNode = Cast<UKzEdGraphNode>(Node))
	{
		KzGraphEditor::DoubleClickNode(*KzNode);
	}
}

void FKzGraphEditor::OnNodeTitleCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphNode* Node)
{
	if (Node)
	{
		const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Node"));
		Node->Modify();
		Node->OnRenameNode(Text.ToString());
	}
}

FReply FKzGraphEditor::OnSpawnNodeByShortcut(FInputChord Chord, const FVector2f& Location)
{
	UEdGraph* EdGraph = GraphEditor.IsValid() ? GraphEditor->GetCurrentGraph() : nullptr;
	const UScriptStruct* NodeStruct = EdGraph && Graph.IsValid() && !Chord.HasAnyModifierKeys() ? UKzEdGraphSchema::FindNodeStructForShortcut(*Graph, Chord.Key) : nullptr;
	if (!NodeStruct)
	{
		return FReply::Unhandled();
	}

	FKzGraphSchemaAction_NewNode Action;
	Action.NodeStruct = NodeStruct;
	Action.PerformAction(EdGraph, nullptr, Location);
	return FReply::Handled();
}

void FKzGraphEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	if (Action.Action & GRAPHACTION_SelectNode)
	{
		return;
	}

	if (FindResults.IsValid())
	{
		FindResults->Refresh();
	}
	if (Preview.IsValid())
	{
		Preview->OnGraphChanged();
	}
}

void FKzGraphEditor::Compile()
{
	if (UKzEdGraph* EdGraph = GetEdGraph())
	{
		EdGraph->ReconstructNodes();
		EdGraph->Compile();
		if (!Issues.IsEmpty())
		{
			TabManager->TryInvokeTab(CompileResultsTabId);
		}
	}
}

TArray<FKzValidationIssue> FKzGraphEditor::CompileForResults()
{
	Compile();
	return Issues;
}

void FKzGraphEditor::OnCompiled()
{
	if (const UKzEdGraph* EdGraph = GetEdGraph())
	{
		ShowIssues(EdGraph->GetCompileIssues());
	}
}

void FKzGraphEditor::ShowIssues(const TArray<FKzValidationIssue>& InIssues)
{
	Issues = InIssues;
	CompileResults->SetIssues(Issues);
}

void FKzGraphEditor::OnIssueActivated(const FKzValidationIssue& Issue)
{
	const UKzEdGraph* EdGraph = GetEdGraph();
	const TObjectPtr<UEdGraphNode>* Node = EdGraph && Issue.ContextId.IsValid() ? EdGraph->Nodes.FindByPredicate([&Issue](const UEdGraphNode* Each) { return Each->NodeGuid == Issue.ContextId; }) : nullptr;
	if (Node)
	{
		JumpToNode(*Node);
	}
}

EKzValidationSeverity FKzGraphEditor::GetWorstSeverity() const
{
	EKzValidationSeverity Worst = EKzValidationSeverity::Info;
	for (const FKzValidationIssue& Issue : Issues)
	{
		Worst = FMath::Max(Worst, Issue.Severity);
	}
	return Worst;
}

FSlateIcon FKzGraphEditor::GetCompileIcon() const
{
	// As in Blueprints, uncompiled edits show a question mark over the result of the last compile.
	const UKzEdGraph* EdGraph = GetEdGraph();
	if (EdGraph && EdGraph->NeedsCompile())
	{
		return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("AssetEditor.CompileStatus.Background"), NAME_None, TEXT("AssetEditor.CompileStatus.Overlay.Unknown"));
	}

	const EKzValidationSeverity Worst = GetWorstSeverity();
	const FName Overlay = Worst == EKzValidationSeverity::Error ? TEXT("AssetEditor.CompileStatus.Overlay.Error") : Worst == EKzValidationSeverity::Warning ? TEXT("AssetEditor.CompileStatus.Overlay.Warning") : TEXT("AssetEditor.CompileStatus.Overlay.Good");
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("AssetEditor.CompileStatus.Background"), NAME_None, Overlay);
}

FText FKzGraphEditor::GetCompileTooltip() const
{
	const UKzEdGraph* EdGraph = GetEdGraph();
	if (EdGraph && EdGraph->NeedsCompile())
	{
		return LOCTEXT("CompileNeeded", "Not compiled: the game still runs the last compile. (F7)");
	}

	switch (GetWorstSeverity())
	{
	case EKzValidationSeverity::Error:
		return LOCTEXT("CompileError", "Some nodes have errors; see the compile results. (F7)");
	case EKzValidationSeverity::Warning:
		return LOCTEXT("CompileWarning", "Some nodes have warnings; see the compile results. (F7)");
	default:
		return LOCTEXT("CompileGood", "Good to go. (F7)");
	}
}

bool FKzGraphEditor::IsAutoCompile() const
{
	bool bAutoCompile = true;
	GConfig->GetBool(SettingsSection, AutoCompileKey, bAutoCompile, GEditorPerProjectIni);
	return bAutoCompile;
}

void FKzGraphEditor::ToggleAutoCompile()
{
	GConfig->SetBool(SettingsSection, AutoCompileKey, !IsAutoCompile(), GEditorPerProjectIni);
}

TSharedRef<SWidget> FKzGraphEditor::MakeCompileOptions()
{
	FMenuBuilder MenuBuilder(true, GetToolkitCommands());
	MenuBuilder.AddMenuEntry(FKzGraphEditorCommands::Get().AutoCompile);
	return MenuBuilder.MakeWidget();
}

bool FKzGraphEditor::TickAutoCompile(float DeltaTime)
{
	// Waits for the mouse to be let go, so a drag compiles once at its end.
	UKzEdGraph* EdGraph = GetEdGraph();
	if (EdGraph && EdGraph->NeedsCompile() && IsAutoCompile() && FSlateApplication::Get().GetPressedMouseButtons().IsEmpty())
	{
		EdGraph->Compile();
	}
	return true;
}

void FKzGraphEditor::FindInGraph()
{
	TabManager->TryInvokeTab(FindTabId);
	FindResults->FocusSearch();
}

void FKzGraphEditor::StraightenConnections()
{
	if (GraphEditor.IsValid())
	{
		KzGraphEditor::StraightenConnections(*GraphEditor);
	}
}

void FKzGraphEditor::AddGraphEditorPinActionsToContextMenu(FToolMenuSection& InSection) const
{
	if (!Graph.IsValid() || Graph->GetFlow() != EKzGraphFlow::Vertical || !GraphEditor.IsValid())
	{
		return;
	}

	// Replaces the engine's submenu of the same name, which straightens across a horizontal flow.
	InSection.RemoveEntry(TEXT("StraightenPinConnection"));
	const UGraphNodeContextMenuContext* NodeContext = InSection.FindContext<UGraphNodeContextMenuContext>();
	if (!NodeContext || !NodeContext->Pin || NodeContext->Pin->LinkedTo.IsEmpty())
	{
		return;
	}

	const TWeakPtr<SGraphEditor> WeakEditor = GraphEditor;
	const FEdGraphPinReference PinReference(NodeContext->Pin);
	InSection.AddSubMenu(TEXT("StraightenPinConnection"), LOCTEXT("StraightenPin", "Straighten Connection..."), LOCTEXT("StraightenPinTip", "Straighten specific connection..."), FNewToolMenuDelegate::CreateLambda([WeakEditor, PinReference](UToolMenu* Menu)
	{
		UEdGraphPin* Pin = PinReference.Get();
		if (!Pin)
		{
			return;
		}

		FToolMenuSection& Section = Menu->AddSection(TEXT("KzStraightenPin"));
		auto AddEntry = [&Section, WeakEditor, PinReference](const FEdGraphPinReference& OnlyTo, const FText& Label)
		{
			Section.AddMenuEntry(NAME_None, Label, FText::GetEmpty(), FGraphEditorCommands::Get().StraightenConnections->GetIcon(), FUIAction(FExecuteAction::CreateLambda([WeakEditor, PinReference, OnlyTo]()
			{
				const TSharedPtr<SGraphEditor> Editor = WeakEditor.Pin();
				if (UEdGraphPin* Anchor = PinReference.Get(); Editor.IsValid() && Anchor)
				{
					KzGraphEditor::StraightenPinConnections(*Editor, *Anchor, OnlyTo.Get());
				}
			})));
		};

		AddEntry(FEdGraphPinReference(), LOCTEXT("StraightenAllPin", "Straighten All Pin Connections"));
		for (UEdGraphPin* Linked : Pin->LinkedTo)
		{
			if (Linked)
			{
				AddEntry(FEdGraphPinReference(Linked), FText::Format(LOCTEXT("StraightenPinTo", "Straighten Connection to {0}"), Linked->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView)));
			}
		}
	}));
}

FReply FKzGraphEditor::OnGraphPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// A text edited inside the graph, such as a comment, keeps its letters.
	static const FName EditableText(TEXT("SEditableText"));
	static const FName MultiLineEditableText(TEXT("SMultiLineEditableText"));
	const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (Focused.IsValid() && (Focused->GetType() == EditableText || Focused->GetType() == MultiLineEditableText))
	{
		return FReply::Unhandled();
	}
	return PreviewCommands.IsValid() && PreviewCommands->ProcessCommandBindings(KeyEvent) ? FReply::Handled() : FReply::Unhandled();
}

void FKzGraphEditor::DeleteSelectedNodes(bool bReconnect)
{
	const FScopedTransaction Transaction(bReconnect ? FGraphEditorCommands::Get().DeleteAndReconnectNodes->GetDescription() : FGenericCommands::Get().Delete->GetDescription());
	UEdGraph* EdGraph = GraphEditor->GetCurrentGraph();
	EdGraph->Modify();

	const UEdGraphSchema* Schema = EdGraph->GetSchema();
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (!Node || !Node->CanUserDeleteNode())
		{
			continue;
		}

		Node->Modify();

		const TArray<UEdGraphPin*> Inputs = Node->Pins.FilterByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Input; });
		const TArray<UEdGraphPin*> Outputs = Node->Pins.FilterByPredicate([](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Output; });
		if (bReconnect && Inputs.Num() == 1 && Outputs.Num() == 1)
		{
			// Unlinked first, so the limits of the pins it fed leave room for the new links.
			const TArray<UEdGraphPin*> Sources = Inputs[0]->LinkedTo;
			const TArray<UEdGraphPin*> Targets = Outputs[0]->LinkedTo;
			Node->BreakAllNodeLinks();
			for (UEdGraphPin* Source : Sources)
			{
				for (UEdGraphPin* Target : Targets)
				{
					Schema->TryCreateConnection(Source, Target);
				}
			}
		}

		Node->DestroyNode();
	}

	GraphEditor->ClearSelectionSet();
}

bool FKzGraphEditor::CanDeleteNodes() const
{
	if (!GraphEditor.IsValid())
	{
		return false;
	}

	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}
	return false;
}

void FKzGraphEditor::CopySelectedNodes()
{
	TSet<UObject*> Copyable;
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (Node && Node->CanDuplicateNode())
		{
			Node->PrepareForCopying();
			Copyable.Add(Node);
		}
	}

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(Copyable, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FKzGraphEditor::CanCopyNodes() const
{
	if (!GraphEditor.IsValid())
	{
		return false;
	}

	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FKzGraphEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedNodes(false);
}

bool FKzGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FKzGraphEditor::PasteNodes()
{
	UEdGraph* EdGraph = GraphEditor->GetCurrentGraph();
	const FVector2f Location = GraphEditor->GetPasteLocation2f();

	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	EdGraph->Modify();
	GraphEditor->ClearSelectionSet();

	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

	// The pasted nodes keep their layout, centered on the paste location.
	FVector2f Center = FVector2f::ZeroVector;
	for (const UEdGraphNode* Node : PastedNodes)
	{
		Center += FVector2f(static_cast<float>(Node->NodePosX), static_cast<float>(Node->NodePosY));
	}
	if (!PastedNodes.IsEmpty())
	{
		Center /= static_cast<float>(PastedNodes.Num());
	}

	for (UEdGraphNode* Node : PastedNodes)
	{
		Node->NodePosX = FMath::RoundToInt(Node->NodePosX - Center.X + Location.X);
		Node->NodePosY = FMath::RoundToInt(Node->NodePosY - Center.Y + Location.Y);
		Node->SnapToGrid(SNodePanel::GetSnapGridSize());
		Node->CreateNewGuid();
		GraphEditor->SetNodeSelection(Node, true);
	}

	EdGraph->NotifyGraphChanged();
}

bool FKzGraphEditor::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	return GraphEditor.IsValid() && FEdGraphUtilities::CanImportNodesFromText(GraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FKzGraphEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

void FKzGraphEditor::RenameSelectedNode()
{
	const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
	if (UEdGraphNode* Node = Selection.Num() == 1 ? Cast<UEdGraphNode>(*Selection.CreateConstIterator()) : nullptr)
	{
		GraphEditor->IsNodeTitleVisible(Node, true);
	}
}

bool FKzGraphEditor::CanRenameNode() const
{
	if (!GraphEditor.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet Selection = GraphEditor->GetSelectedNodes();
	const UEdGraphNode* Node = Selection.Num() == 1 ? Cast<UEdGraphNode>(*Selection.CreateConstIterator()) : nullptr;
	return Node && Node->GetCanRenameNode();
}

void FKzGraphEditor::BreakSelectedNodeLinks()
{
	const FScopedTransaction Transaction(FGraphEditorCommands::Get().BreakNodeLinks->GetDescription());
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			Node->Modify();
			Node->GetSchema()->BreakNodeLinks(*Node);
		}
	}
	GraphEditor->GetCurrentGraph()->NotifyGraphChanged();
}

void FKzGraphEditor::ReconstructSelectedNodes()
{
	const FScopedTransaction Transaction(FGraphEditorCommands::Get().ReconstructNodes->GetDescription());
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
		{
			Node->ReconstructNode();
		}
	}
	GraphEditor->GetCurrentGraph()->NotifyGraphChanged();
}

void FKzGraphEditor::CreateComment()
{
	UEdGraph* EdGraph = GraphEditor->GetCurrentGraph();
	if (const TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction())
	{
		Action->PerformAction(EdGraph, nullptr, GraphEditor->GetPasteLocation2f());
	}
}

#undef LOCTEXT_NAMESPACE
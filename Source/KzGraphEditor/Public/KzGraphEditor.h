// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Core/KzValidationTypes.h"
#include "EditorUndoClient.h"
#include "Framework/Commands/Commands.h"
#include "Toolkits/AssetEditorToolkit.h"

class FKzGraphPreview;
class IDetailsView;
class SGraphEditor;
class SKzGraphFindResults;
class SKzValidationPanel;
class UEdGraphNode;
class UKzEdGraph;
class UKzGraph;
struct FEdGraphEditAction;
struct FInputChord;

/** Toolbar and shortcut commands of graph editors. */
class KZGRAPHEDITOR_API FKzGraphEditorCommands : public TCommands<FKzGraphEditorCommands>
{
public:
	FKzGraphEditorCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Compile;
	TSharedPtr<FUICommandInfo> AutoCompile;
	TSharedPtr<FUICommandInfo> Find;
};

/**
 * Asset editor of any graph: the graph, the details of the selection or of the asset, search and compile results, and a viewport for the kinds of graph that preview something.
 * The graph takes the engine's usual commands: clipboard, deletion, comments, alignment, and a node's Shortcut key held while clicking to place it.
 * Kinds of graph subclass it for a viewport, with their own static CreateEditor to register.
 */
class KZGRAPHEDITOR_API FKzGraphEditor : public FAssetEditorToolkit, public FSelfRegisteringEditorUndoClient
{
public:
	/** Opens an editor on each graph asset, as TKzAssetTypeActions asks for. */
	static void CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, const TArray<UObject*>& Objects);

	virtual ~FKzGraphEditor() override;

	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UKzGraph* InGraph);

	/** Brings the graph forward framed and selected on the node. */
	void JumpToNode(UEdGraphNode* Node);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

protected:
	/** What the viewport shows; none leaves the editor without one. */
	virtual TSharedPtr<FKzGraphPreview> CreatePreview() { return nullptr; }

	UKzGraph* GetGraphAsset() const { return Graph.Get(); }

private:
	UKzEdGraph* GetEdGraph() const;

	void BindCommands();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);

	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnCompileResultsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnFindTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);

	void OnSelectionChanged(const TSet<UObject*>& Selection);
	void OnNodeTitleCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphNode* Node);
	FReply OnSpawnNodeByShortcut(FInputChord Chord, const FVector2f& Location);
	void OnGraphChanged(const FEdGraphEditAction& Action);

	/** Rebuilds the nodes from their structs, picking up code changed since they loaded, and compiles. */
	void Compile();
	TArray<FKzValidationIssue> CompileForResults();
	void OnCompiled();
	void ShowIssues(const TArray<FKzValidationIssue>& InIssues);
	void OnIssueActivated(const FKzValidationIssue& Issue);
	EKzValidationSeverity GetWorstSeverity() const;
	FSlateIcon GetCompileIcon() const;
	FText GetCompileTooltip() const;

	/** Whether edits compile on their own once the mouse is let go: a per-user preference shared by every graph editor. */
	bool IsAutoCompile() const;
	void ToggleAutoCompile();
	TSharedRef<SWidget> MakeCompileOptions();
	bool TickAutoCompile(float DeltaTime);

	void FindInGraph();

	/** Deletes the selected nodes; reconnecting lets the flow pass each node with one input and one output, as Shift+Delete does in Blueprints. */
	void DeleteSelectedNodes(bool bReconnect);
	bool CanDeleteNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void CutSelectedNodes();
	bool CanCutNodes() const;
	void PasteNodes();
	bool CanPasteNodes() const;
	void DuplicateNodes();
	void RenameSelectedNode();
	bool CanRenameNode() const;
	void BreakSelectedNodeLinks();
	void ReconstructSelectedNodes();
	void CreateComment();

	TWeakObjectPtr<UKzGraph> Graph;
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SKzValidationPanel> CompileResults;
	TSharedPtr<SKzGraphFindResults> FindResults;
	TSharedPtr<FKzGraphPreview> Preview;

	/** The viewport with the preview's panel under it, made once so reopening its tab keeps the scene. */
	TSharedPtr<SWidget> PreviewPanel;
	FDelegateHandle GraphChangedHandle;
	FTSTicker::FDelegateHandle AutoCompileTicker;

	/** Problems of the last compile. They are not saved, so opening the editor finds them again. */
	TArray<FKzValidationIssue> Issues;
};
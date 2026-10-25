// Copyright 2026 kirzo

#include "EdGraphUtilities.h"
#include "KzEdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditor.h"
#include "Modules/ModuleManager.h"
#include "SGraphNodeKnot.h"
#include "SKzGraphNode.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
	class FKzGraphNodeFactory : public FGraphPanelNodeFactory
	{
		virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
		{
			if (UKzEdGraphNode_Reroute* Reroute = Cast<UKzEdGraphNode_Reroute>(Node))
			{
				return SNew(SGraphNodeKnot, Reroute);
			}
			if (UKzEdGraphNode* KzNode = Cast<UKzEdGraphNode>(Node))
			{
				return SNew(SKzGraphNode, KzNode);
			}
			return nullptr;
		}
	};
}

class FKzGraphEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FKzGraphEditorCommands::Register();

		NodeFactory = MakeShared<FKzGraphNodeFactory>();
		FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);

		PreSaveHandle = FCoreUObjectDelegates::OnObjectPreSave.AddRaw(this, &FKzGraphEditorModule::OnObjectPreSave);
	}

	virtual void ShutdownModule() override
	{
		FCoreUObjectDelegates::OnObjectPreSave.Remove(PreSaveHandle);
		FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
		FKzGraphEditorCommands::Unregister();
	}

private:
	/** A graph saves with its data compiled from the latest edits. */
	void OnObjectPreSave(UObject* Object, FObjectPreSaveContext Context)
	{
		const UKzGraph* Asset = Cast<UKzGraph>(Object);
		UKzEdGraph* EdGraph = Asset ? Cast<UKzEdGraph>(Asset->EdGraph) : nullptr;
		if (EdGraph && EdGraph->NeedsCompile())
		{
			EdGraph->Compile();
		}
	}

	TSharedPtr<FGraphPanelNodeFactory> NodeFactory;
	FDelegateHandle PreSaveHandle;
};

IMPLEMENT_MODULE(FKzGraphEditorModule, KzGraphEditor)
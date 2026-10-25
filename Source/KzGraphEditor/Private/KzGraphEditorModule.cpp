// Copyright 2026 kirzo

#include "EdGraphUtilities.h"
#include "KzEdGraphNode.h"
#include "Modules/ModuleManager.h"
#include "SGraphNodeKnot.h"
#include "SKzGraphNode.h"

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
		NodeFactory = MakeShared<FKzGraphNodeFactory>();
		FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);
	}

	virtual void ShutdownModule() override
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
	}

private:
	TSharedPtr<FGraphPanelNodeFactory> NodeFactory;
};

IMPLEMENT_MODULE(FKzGraphEditorModule, KzGraphEditor)
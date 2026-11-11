// Copyright 2026 kirzo

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "EdGraphUtilities.h"
#include "IDetailCustomization.h"
#include "InstancedStructDetails.h"
#include "KzEdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SGraphNodeKnot.h"
#include "SKzGraphNode.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
	/** A node's own properties, without the row picking its struct: a node never changes what it is. */
	class FKzEdGraphNodeDetails : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FKzEdGraphNodeDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			const TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UKzEdGraphNode, Node));
			DetailBuilder.HideProperty(NodeHandle);

			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder.GetObjectsBeingCustomized(Objects);
			const UKzEdGraphNode* First = Objects.IsEmpty() ? nullptr : Cast<UKzEdGraphNode>(Objects[0].Get());
			const UScriptStruct* Struct = First && First->Node.IsValid() ? First->Node.GetScriptStruct() : nullptr;
			if (!Struct || !HasEditableProperties(*Struct))
			{
				return;
			}

			DetailBuilder.EditCategory(TEXT("Node"), First->GetTitle()).AddCustomBuilder(MakeShared<FInstancedStructDataDetails>(NodeHandle));
		}

	private:
		static bool HasEditableProperties(const UScriptStruct& Struct)
		{
			for (TFieldIterator<FProperty> It(&Struct); It; ++It)
			{
				if (It->HasAnyPropertyFlags(CPF_Edit))
				{
					return true;
				}
			}
			return false;
		}
	};

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

		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout(UKzEdGraphNode::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FKzEdGraphNodeDetails::MakeInstance));

		PreSaveHandle = FCoreUObjectDelegates::OnObjectPreSave.AddRaw(this, &FKzGraphEditorModule::OnObjectPreSave);
		PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FKzGraphEditorModule::OnObjectPropertyChanged);
	}

	virtual void ShutdownModule() override
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
		FCoreUObjectDelegates::OnObjectPreSave.Remove(PreSaveHandle);
		if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyEditor->UnregisterCustomClassLayout(UKzEdGraphNode::StaticClass()->GetFName());
		}
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

	/** Node pins and required entries can follow the asset's settings, so editing them rebuilds the nodes. */
	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
	{
		const UKzGraph* Asset = Cast<UKzGraph>(Object);
		UKzEdGraph* EdGraph = Asset ? Cast<UKzEdGraph>(Asset->EdGraph) : nullptr;
		if (EdGraph)
		{
			EdGraph->SyncRequiredEntries();
			EdGraph->ReconstructNodes();
		}
	}

	TSharedPtr<FGraphPanelNodeFactory> NodeFactory;
	FDelegateHandle PreSaveHandle;
	FDelegateHandle PropertyChangedHandle;
};

IMPLEMENT_MODULE(FKzGraphEditorModule, KzGraphEditor)
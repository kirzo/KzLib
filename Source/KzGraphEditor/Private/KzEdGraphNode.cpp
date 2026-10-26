// Copyright 2026 kirzo

#include "KzEdGraphNode.h"
#include "KzEdGraph.h"
#include "KzEdGraphSchema.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"

#define LOCTEXT_NAMESPACE "KzEdGraphNode"

bool UKzEdGraphNodeBase::Modify(bool bAlwaysMarkDirty)
{
	if (UKzEdGraph* Graph = Cast<UKzEdGraph>(GetOuter()))
	{
		Graph->MarkChanged();
	}
	return Super::Modify(bAlwaysMarkDirty);
}

void UKzEdGraphNodeBase::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	NotifyGraphChanged();
}

void UKzEdGraphNodeBase::PostEditUndo()
{
	Super::PostEditUndo();
	NotifyGraphChanged();
}

bool UKzEdGraphNodeBase::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema && Schema->IsA<UKzEdGraphSchema>();
}

void UKzEdGraphNodeBase::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin)
	{
		return;
	}

	const UEdGraphSchema* Schema = GetSchema();
	for (UEdGraphPin* Pin : Pins)
	{
		if (Schema->CanCreateConnection(FromPin, Pin).Response != CONNECT_RESPONSE_DISALLOW)
		{
			Schema->TryCreateConnection(FromPin, Pin);
			return;
		}
	}
}

void UKzEdGraphNodeBase::ReconstructNode()
{
	// Mid paste, links to nodes not imported yet are null until the paste resolves them into these very pins.
	if (Pins.ContainsByPredicate([](const UEdGraphPin* Pin) { return Pin->LinkedTo.Contains(nullptr); }))
	{
		return;
	}

	Modify();

	TArray<UEdGraphPin*> OldPins = MoveTemp(Pins);
	Pins.Reset();
	AllocateDefaultPins();

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (UEdGraphPin* NewPin = FindPin(OldPin->PinName, OldPin->Direction))
		{
			NewPin->MovePersistentDataFromOldPin(*OldPin);
		}
		else
		{
			OldPin->BreakAllPinLinks();
		}

		OldPin->Modify();
		DestroyPin(OldPin);
	}
}

void UKzEdGraphNodeBase::NotifyGraphChanged() const
{
	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
}

void UKzEdGraphNode::AllocateDefaultPins()
{
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(GetGraph());
	if (!Node.IsValid() || !Asset)
	{
		return;
	}

	TArray<FKzGraphPinDesc> Inputs;
	TArray<FKzGraphPinDesc> Outputs;
	Node.Get().GetInputPins(*Asset, Inputs);
	Node.Get().GetOutputPins(*Asset, Outputs);

	// A lone pin on its side needs no label.
	auto AddPins = [this](EEdGraphPinDirection Direction, const TArray<FKzGraphPinDesc>& Descs)
	{
		for (const FKzGraphPinDesc& Desc : Descs)
		{
			UEdGraphPin* Pin = CreatePin(Direction, Desc.Category.IsNone() ? UKzEdGraphSchema::PC_Flow : Desc.Category, Desc.Name);
			Pin->PinFriendlyName = !Desc.DisplayName.IsEmpty() ? Desc.DisplayName : Descs.Num() > 1 ? FText::FromName(Desc.Name) : FText::GetEmpty();
			Pin->PinToolTip = Desc.ToolTip.ToString();
		}
	};
	AddPins(EGPD_Input, Inputs);
	AddPins(EGPD_Output, Outputs);
}

FText UKzEdGraphNode::GetTitle() const
{
	if (!Node.IsValid())
	{
		return LOCTEXT("MissingNode", "Missing Node");
	}

	const FText Title = Node.Get().GetTitle();
	return Title.IsEmpty() ? Node.GetScriptStruct()->GetDisplayNameText() : Title;
}

FText UKzEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FText Title = GetTitle();
	const FText Subtitle = Node.IsValid() ? Node.Get().GetSubtitle() : FText::GetEmpty();
	if (TitleType != ENodeTitleType::FullTitle || Subtitle.IsEmpty())
	{
		return Title;
	}
	return FText::Format(LOCTEXT("TitleWithSubtitle", "{0}\n{1}"), Title, Subtitle);
}

FText UKzEdGraphNode::GetTooltipText() const
{
	return Node.IsValid() ? Node.GetScriptStruct()->GetToolTipText() : FText::GetEmpty();
}

FLinearColor UKzEdGraphNode::GetNodeTitleColor() const
{
	return Node.IsValid() ? Node.Get().GetColor() : FLinearColor::Red;
}

FSlateIcon UKzEdGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return Node.IsValid() ? Node.Get().GetIcon() : FSlateIcon();
}

bool UKzEdGraphNode::CanUserDeleteNode() const
{
	return !Node.IsValid() || Node.Get().CanUserDelete();
}

bool UKzEdGraphNode::CanDuplicateNode() const
{
	return Node.IsValid() && Node.Get().CanDuplicate();
}

bool UKzEdGraphNode::CanPasteHere(const UEdGraph* TargetGraph) const
{
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(TargetGraph);
	return Asset && Node.IsValid() && Node.GetScriptStruct()->IsChildOf(Asset->GetNodeStruct());
}

void UKzEdGraphNode::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (Node.IsValid())
	{
		Node.GetMutable().PostEditChange();
	}

	// Pins can follow the node's properties, such as one output per option.
	ReconstructNode();
	Super::PostEditChangeProperty(Event);
}

UKzEdGraphNode_Reroute* UKzEdGraphNode_Reroute::Create(UEdGraph& Graph, const FVector2f& Location, bool bSelect)
{
	FGraphNodeCreator<UKzEdGraphNode_Reroute> Creator(Graph);
	UKzEdGraphNode_Reroute* Reroute = Creator.CreateNode(bSelect);
	Reroute->NodePosX = static_cast<int32>(Location.X);
	Reroute->NodePosY = static_cast<int32>(Location.Y);
	Creator.Finalize();
	return Reroute;
}

void UKzEdGraphNode_Reroute::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UKzEdGraphSchema::PC_Flow, TEXT("In"));
	CreatePin(EGPD_Output, UKzEdGraphSchema::PC_Flow, TEXT("Out"));
}

FText UKzEdGraphNode_Reroute::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TitleType == ENodeTitleType::EditableTitle ? FText::FromString(NodeComment) : LOCTEXT("RerouteTitle", "Reroute Node");
}

bool UKzEdGraphNode_Reroute::ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const
{
	OutInputPinIndex = 0;
	OutOutputPinIndex = 1;
	return true;
}

UEdGraphPin* UKzEdGraphNode_Reroute::GetPassThroughPin(const UEdGraphPin* FromPin) const
{
	return FromPin == Pins[0] ? Pins[1] : FromPin == Pins[1] ? Pins[0] : nullptr;
}

#undef LOCTEXT_NAMESPACE
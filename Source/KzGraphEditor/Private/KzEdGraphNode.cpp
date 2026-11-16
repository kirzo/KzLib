// Copyright 2026 kirzo

#include "KzEdGraphNode.h"
#include "KzEdGraph.h"
#include "KzEdGraphSchema.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "KzEdGraphNode"

namespace
{
	const FLinearColor GoToColor(0.08f, 0.16f, 0.28f);
}

bool UKzEdGraphNodeBase::Modify(bool bAlwaysMarkDirty)
{
	MarkGraphChanged();
	return Super::Modify(bAlwaysMarkDirty);
}

void UKzEdGraphNodeBase::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	MarkGraphChanged();

	// A dragged value changes on every mouse move, and the graph's widgets rebuild on each notify: once let go is enough.
	if (Event.ChangeType != EPropertyChangeType::Interactive)
	{
		NotifyGraphChanged();
	}
}

void UKzEdGraphNodeBase::PostEditUndo()
{
	Super::PostEditUndo();
	MarkGraphChanged();
	NotifyGraphChanged();
}

void UKzEdGraphNodeBase::MarkGraphChanged() const
{
	if (UKzEdGraph* Graph = Cast<UKzEdGraph>(GetOuter()))
	{
		Graph->MarkChanged();
	}
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

FText UKzEdGraphNode::GetSubtitle() const
{
	return Node.IsValid() ? Node.Get().GetSubtitle() : FText::GetEmpty();
}

FText UKzEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetTitle();
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
	// The paste asks the class default, which holds no node: each pasted node is checked by CanPasteHere.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return true;
	}
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
	if (Event.ChangeType != EPropertyChangeType::Interactive)
	{
		ReconstructNode();
	}
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

UKzEdGraphNode_Label::UKzEdGraphNode_Label()
{
	bCanRenameNode = true;
}

TArray<UKzEdGraphNode_Label*> UKzEdGraphNode_Label::FindAll(const UEdGraph& Graph, FName Name)
{
	TArray<UKzEdGraphNode_Label*> Found;
	for (UEdGraphNode* Node : Graph.Nodes)
	{
		UKzEdGraphNode_Label* Label = Cast<UKzEdGraphNode_Label>(Node);
		if (Label && !Name.IsNone() && Label->LabelName == Name)
		{
			Found.Add(Label);
		}
	}
	return Found;
}

void UKzEdGraphNode_Label::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, UKzEdGraphSchema::PC_Flow, TEXT("Out"));
}

FText UKzEdGraphNode_Label::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LabelName.IsNone() ? LOCTEXT("LabelTitle", "Label") : FText::FromName(LabelName);
}

FText UKzEdGraphNode_Label::GetSubtitle() const
{
	return LabelName.IsNone() ? FText::GetEmpty() : LOCTEXT("LabelTitle", "Label");
}

FLinearColor UKzEdGraphNode_Label::GetNodeTitleColor() const
{
	return GoToColor;
}

FSlateIcon UKzEdGraphNode_Label::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GraphEditor.Bookmark"));
}

void UKzEdGraphNode_Label::OnRenameNode(const FString& NewName)
{
	const FName NewLabelName(*NewName.TrimStartAndEnd());
	if (NewLabelName.IsNone() || NewLabelName == LabelName)
	{
		return;
	}

	const FName OldName = LabelName;
	LabelName = NewLabelName;
	RetargetGoTos(OldName);
	MarkGraphChanged();
	NotifyGraphChanged();
}

void UKzEdGraphNode_Label::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	NameBeforeEdit = LabelName;
}

void UKzEdGraphNode_Label::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UKzEdGraphNode_Label, LabelName) && NameBeforeEdit != LabelName)
	{
		RetargetGoTos(NameBeforeEdit);
	}
	Super::PostEditChangeProperty(Event);
}

void UKzEdGraphNode_Label::RetargetGoTos(FName OldName) const
{
	const UEdGraph* Graph = GetGraph();
	if (!Graph || OldName.IsNone() || !FindAll(*Graph, OldName).IsEmpty() || UKzEdGraphNode_GoTo::FindTarget(*Graph, OldName))
	{
		return;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UKzEdGraphNode_GoTo* GoTo = Cast<UKzEdGraphNode_GoTo>(Node);
		if (GoTo && GoTo->Target == OldName)
		{
			GoTo->Modify();
			GoTo->Target = LabelName;
		}
	}
}

UEdGraphNode* UKzEdGraphNode_GoTo::FindTarget(const UEdGraph& Graph, FName Name)
{
	const TArray<UKzEdGraphNode_Label*> Labels = UKzEdGraphNode_Label::FindAll(Graph, Name);
	if (!Labels.IsEmpty())
	{
		return Labels.Num() == 1 ? Labels[0] : nullptr;
	}

	TArray<UKzEdGraphNode_Label*> NoLabels;
	TArray<UKzEdGraphNode*> Entries;
	GetTargets(Graph, NoLabels, Entries);
	UKzEdGraphNode* const* Entry = Entries.FindByPredicate([Name](const UKzEdGraphNode* Each) { return !Name.IsNone() && GetTargetName(*Each) == Name; });
	return Entry ? *Entry : nullptr;
}

UEdGraphNode* UKzEdGraphNode_GoTo::FindTarget() const
{
	const UEdGraph* Graph = GetGraph();
	return Graph ? FindTarget(*Graph, Target) : nullptr;
}

void UKzEdGraphNode_GoTo::GetTargets(const UEdGraph& Graph, TArray<UKzEdGraphNode_Label*>& OutLabels, TArray<UKzEdGraphNode*>& OutEntries)
{
	for (UEdGraphNode* Node : Graph.Nodes)
	{
		if (UKzEdGraphNode_Label* Label = Cast<UKzEdGraphNode_Label>(Node))
		{
			OutLabels.Add(Label);
		}
		else if (UKzEdGraphNode* Entry = Cast<UKzEdGraphNode>(Node); Entry && Entry->Node.IsValid() && !Entry->Node.Get().GetEntryName().IsNone())
		{
			OutEntries.Add(Entry);
		}
	}

	auto ByName = [](const UEdGraphNode& A, const UEdGraphNode& B) { return GetTargetName(A).LexicalLess(GetTargetName(B)); };
	OutLabels.Sort(ByName);
	OutEntries.Sort(ByName);
}

FName UKzEdGraphNode_GoTo::GetTargetName(const UEdGraphNode& Node)
{
	if (const UKzEdGraphNode_Label* Label = Cast<UKzEdGraphNode_Label>(&Node))
	{
		return Label->LabelName;
	}
	const UKzEdGraphNode* Entry = Cast<UKzEdGraphNode>(&Node);
	return Entry && Entry->Node.IsValid() ? Entry->Node.Get().GetEntryName() : NAME_None;
}

void UKzEdGraphNode_GoTo::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UKzEdGraphSchema::PC_Flow, TEXT("In"));
}

FText UKzEdGraphNode_GoTo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("GoToTitle", "Go To");
}

FLinearColor UKzEdGraphNode_GoTo::GetNodeTitleColor() const
{
	return GoToColor;
}

TArray<FString> UKzEdGraphNode_GoTo::GetTargetNames() const
{
	TArray<UKzEdGraphNode_Label*> Labels;
	TArray<UKzEdGraphNode*> Entries;
	if (const UEdGraph* Graph = GetGraph())
	{
		GetTargets(*Graph, Labels, Entries);
	}

	// Entries always have a name; a Label may not yet.
	TArray<FString> Names;
	for (const UKzEdGraphNode_Label* Label : Labels)
	{
		if (!Label->LabelName.IsNone())
		{
			Names.AddUnique(Label->LabelName.ToString());
		}
	}
	for (const UKzEdGraphNode* Entry : Entries)
	{
		Names.AddUnique(GetTargetName(*Entry).ToString());
	}
	return Names;
}

#undef LOCTEXT_NAMESPACE
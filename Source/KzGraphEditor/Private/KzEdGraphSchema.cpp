// Copyright 2026 kirzo

#include "KzEdGraphSchema.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "ConnectionDrawingPolicy.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "KzEdGraph.h"
#include "KzEdGraphNode.h"
#include "KzGraph.h"
#include "KzGraphEditorUtils.h"
#include "ScopedTransaction.h"
#include "SNodePanel.h"
#include "AssetRegistry/AssetData.h"
#include "StructUtils/InstancedStruct.h"
#include "Styling/CoreStyle.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "KzEdGraphSchema"

const FName UKzEdGraphSchema::PC_Flow(TEXT("Flow"));

namespace
{
	bool IsReroute(const UEdGraphPin* Pin)
	{
		return Pin && Pin->GetOwningNode()->IsA<UKzEdGraphNode_Reroute>();
	}

	/** Wires in their pin's color, along the graph's flow, numbered when an output has several. */
	class FKzGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
	{
	public:
		FKzGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, const UKzGraph* InAsset)
			: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
			, Asset(InAsset)
			, Flow(InAsset ? InAsset->GetFlow() : EKzGraphFlow::Horizontal)
		{
			ArrowImage = nullptr;
		}

		virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override
		{
			FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

			const UEdGraphPin* Pin = OutputPin ? OutputPin : InputPin;
			if (Pin && Asset)
			{
				Params.WireColor = Asset->GetPinColor(Pin->PinType.PinCategory);
			}
			Params.WireThickness = 1.5f;

			if (!HoveredPins.IsEmpty())
			{
				ApplyHoverDeemphasis(OutputPin, InputPin, Params.WireThickness, Params.WireColor);
			}
		}

		virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override
		{
			const FVector2f Start = IsReroute(Params.AssociatedPin1) ? FVector2f(FGeometryHelper::CenterOf(StartGeom)) : GetOutputPoint(StartGeom);
			const FVector2f End = IsReroute(Params.AssociatedPin2) ? FVector2f(FGeometryHelper::CenterOf(EndGeom)) : GetInputPoint(EndGeom);
			FConnectionDrawingPolicy::DrawSplineWithArrow(Start, End, Params);
			DrawOrder(Start, End, Params);
		}

		virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2f& StartPoint, const FVector2f& EndPoint, UEdGraphPin* Pin) override
		{
			if (Flow == EKzGraphFlow::Horizontal || !Pin)
			{
				FConnectionDrawingPolicy::DrawPreviewConnector(PinGeometry, StartPoint, EndPoint, Pin);
				return;
			}

			FConnectionParams Params;
			DetermineWiringStyle(Pin, nullptr, Params);
			const bool bFromOutput = Pin->Direction == EGPD_Output;
			const FVector2f Anchor = bFromOutput ? GetOutputPoint(PinGeometry) : GetInputPoint(PinGeometry);
			FConnectionDrawingPolicy::DrawSplineWithArrow(bFromOutput ? Anchor : EndPoint, bFromOutput ? EndPoint : Anchor, Params);
		}

		virtual FVector2f ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const override
		{
			if (Flow == EKzGraphFlow::Horizontal)
			{
				return FConnectionDrawingPolicy::ComputeSplineTangent(Start, End);
			}
			return FVector2f(0.0f, FMath::Max(FMath::Abs(End.Y - Start.Y), 60.0f * ZoomFactor) * 0.75f);
		}

	private:
		FVector2f GetOutputPoint(const FGeometry& Geometry) const
		{
			return Flow == EKzGraphFlow::Horizontal ? FVector2f(FGeometryHelper::VerticalMiddleRightOf(Geometry)) : FVector2f(Geometry.GetAbsolutePositionAtCoordinates(FVector2f(0.5f, 1.0f)));
		}

		FVector2f GetInputPoint(const FGeometry& Geometry) const
		{
			return Flow == EKzGraphFlow::Horizontal ? FVector2f(FGeometryHelper::VerticalMiddleLeftOf(Geometry)) : FVector2f(Geometry.GetAbsolutePositionAtCoordinates(FVector2f(0.5f, 0.0f)));
		}

		/** The place of the link among the links of its output, at the middle of the wire, where links of one output no longer overlap. */
		void DrawOrder(const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
		{
			const UEdGraphPin* Output = Params.AssociatedPin1;
			const UEdGraphPin* Input = Params.AssociatedPin2;
			if (!Output || !Input || Output->Direction != EGPD_Output || Output->LinkedTo.Num() < 2)
			{
				return;
			}

			TArray<UEdGraphPin*> Linked = Output->LinkedTo;
			Linked.RemoveAll([](const UEdGraphPin* Pin) { return !Pin; });
			KzGraphEditor::SortByFlow(Linked, Flow);
			const int32 Order = Linked.IndexOfByKey(Input);
			if (Order == INDEX_NONE)
			{
				return;
			}

			// Both ends share one tangent, so the spline's middle is the middle of its ends.
			static const FSlateRoundedBoxBrush BadgeBrush(FLinearColor::White, 4.0f);
			const FString Number = FString::FromInt(Order + 1);
			const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 8);
			const FVector2f TextSize = FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Number, Font, ZoomFactor));
			const FVector2f BadgeSize = TextSize + FVector2f(8.0f, 2.0f) * ZoomFactor;
			const FVector2f Middle = (Start + End) * 0.5f;

			FSlateDrawElement::MakeBox(DrawElementsList, WireLayerID + 1, FPaintGeometry(Middle - BadgeSize * 0.5f, BadgeSize, ZoomFactor), &BadgeBrush, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.02f, 0.9f));
			FSlateDrawElement::MakeText(DrawElementsList, WireLayerID + 2, FPaintGeometry(Middle - TextSize * 0.5f, TextSize, ZoomFactor), Number, Font, ESlateDrawEffect::None, Params.WireColor);
		}

		const UKzGraph* Asset;
		EKzGraphFlow Flow;
	};

	/** True when a node of the struct has a pin facing the one dragged from. */
	bool HasPinFacing(const UKzGraph& Graph, const UScriptStruct* NodeStruct, EEdGraphPinDirection FromDirection)
	{
		FInstancedStruct Probe;
		Probe.InitializeAs(NodeStruct);
		TArray<FKzGraphPinDesc> Pins;
		if (FromDirection == EGPD_Output)
		{
			Probe.Get<FKzGraphNode>().GetInputPins(Graph, Pins);
		}
		else
		{
			Probe.Get<FKzGraphNode>().GetOutputPins(Graph, Pins);
		}
		return !Pins.IsEmpty();
	}
}

void UKzEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(&Graph);
	if (!Asset)
	{
		return;
	}

	TArray<const UScriptStruct*> Defaults;
	Asset->GetDefaultNodes(Defaults);

	FVector2f Location = FVector2f::ZeroVector;
	for (const UScriptStruct* NodeStruct : Defaults)
	{
		UKzEdGraphNode* Node = CreateNode(Graph, NodeStruct, Location, false);
		SetNodeMetaData(Node, FNodeMetadata::DefaultGraphNode);
		(Asset->GetFlow() == EKzGraphFlow::Horizontal ? Location.Y : Location.X) += 200.0f;
	}
}

void UKzEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(ContextMenuBuilder.CurrentGraph);
	if (!Asset)
	{
		return;
	}

	const UEdGraphPin* FromPin = ContextMenuBuilder.FromPin;
	ForEachNodeStruct(*Asset, [&ContextMenuBuilder, FromPin, Asset](const UScriptStruct* NodeStruct)
	{
		if (FromPin && !HasPinFacing(*Asset, NodeStruct, FromPin->Direction))
		{
			return;
		}

		const TSharedPtr<FKzGraphSchemaAction_NewNode> Action = MakeShared<FKzGraphSchemaAction_NewNode>(FText::FromString(NodeStruct->GetMetaData(TEXT("Category"))), NodeStruct->GetDisplayNameText(), NodeStruct->GetToolTipText(), 0, FText::FromString(NodeStruct->GetMetaData(TEXT("Keywords"))));
		Action->NodeStruct = NodeStruct;
		ContextMenuBuilder.AddAction(Action);
	});

	if (!FromPin)
	{
		ContextMenuBuilder.AddAction(GetCreateCommentAction());
	}
	ContextMenuBuilder.AddAction(MakeShared<FKzGraphSchemaAction_Reroute>(FText::GetEmpty(), LOCTEXT("AddReroute", "Add Reroute Node"), LOCTEXT("AddRerouteTooltip", "Adds a dot to bend wires through. Double-clicking a wire adds one there."), 0));
}

void UKzEdGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetContextMenuActions(Menu, Context);

	if (!Context->Node || Context->Pin)
	{
		return;
	}

	FToolMenuSection& NodeSection = Menu->AddSection(TEXT("KzGraphNodeActions"), LOCTEXT("NodeActionsHeader", "Node Actions"));
	NodeSection.AddMenuEntry(FGenericCommands::Get().Delete);
	NodeSection.AddMenuEntry(FGenericCommands::Get().Cut);
	NodeSection.AddMenuEntry(FGenericCommands::Get().Copy);
	NodeSection.AddMenuEntry(FGenericCommands::Get().Duplicate);
	if (Context->Node->GetCanRenameNode())
	{
		NodeSection.AddMenuEntry(FGenericCommands::Get().Rename);
	}
	NodeSection.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
	NodeSection.AddMenuEntry(FGraphEditorCommands::Get().ReconstructNodes);

	FToolMenuSection& OrganizationSection = Menu->AddSection(TEXT("KzGraphOrganization"), LOCTEXT("OrganizationHeader", "Organization"));
	OrganizationSection.AddMenuEntry(FGraphEditorCommands::Get().CreateComment);
	OrganizationSection.AddSubMenu(TEXT("Alignment"), LOCTEXT("AlignmentHeader", "Alignment"), FText(), FNewToolMenuDelegate::CreateLambda([](UToolMenu* AlignmentMenu)
	{
		FToolMenuSection& AlignSection = AlignmentMenu->AddSection(TEXT("KzGraphAlign"), LOCTEXT("AlignHeader", "Align"));
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesTop);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesMiddle);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesBottom);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesLeft);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesCenter);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesRight);
		AlignSection.AddMenuEntry(FGraphEditorCommands::Get().StraightenConnections);

		FToolMenuSection& DistributeSection = AlignmentMenu->AddSection(TEXT("KzGraphDistribute"), LOCTEXT("DistributeHeader", "Distribute"));
		DistributeSection.AddMenuEntry(FGraphEditorCommands::Get().DistributeNodesHorizontally);
		DistributeSection.AddMenuEntry(FGraphEditorCommands::Get().DistributeNodesVertically);
	}));
}

const FPinConnectionResponse UKzEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "Both pins are on the same node."));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Connect an output to an input."));
	}

	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	const UEdGraphPin* Input = A->Direction == EGPD_Input ? A : B;
	if (Output->LinkedTo.Contains(Input))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("AlreadyLinked", "These pins are already linked."));
	}

	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(A->GetOwningNode()->GetGraph());
	if (!Asset)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
	}

	// Reroutes connect freely; the compile checks the nodes they end up joining.
	const UKzEdGraphNode* From = Cast<UKzEdGraphNode>(Output->GetOwningNode());
	const UKzEdGraphNode* To = Cast<UKzEdGraphNode>(Input->GetOwningNode());
	FText Reason;
	if (From && To && From->Node.IsValid() && To->Node.IsValid() && !Asset->CanConnect(FConstStructView(From->Node.GetScriptStruct(), From->Node.GetMemory()), Output->PinName, FConstStructView(To->Node.GetScriptStruct(), To->Node.GetMemory()), Input->PinName, Reason))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, Reason);
	}

	const int32 MaxPerOutput = IsReroute(Output) ? 0 : Asset->GetMaxLinksPerOutput();
	const int32 MaxPerInput = IsReroute(Input) ? 0 : Asset->GetMaxLinksPerInput();
	const bool bOutputFull = MaxPerOutput > 0 && Output->LinkedTo.Num() >= MaxPerOutput;
	const bool bInputFull = MaxPerInput > 0 && Input->LinkedTo.Num() >= MaxPerInput;

	// A pin that takes a single link swaps it for the new one; one that takes more stops when full.
	if (bOutputFull && bInputFull)
	{
		return MaxPerOutput == 1 && MaxPerInput == 1 ? FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("ReplaceBoth", "Replace the existing links.")) : FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("BothFull", "Both pins already have all the links they take."));
	}
	if (bOutputFull)
	{
		return MaxPerOutput == 1 ? FPinConnectionResponse(Output == A ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("ReplaceOutput", "Replace the existing link.")) : FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("OutputFull", "This output already has all the links it takes."));
	}
	if (bInputFull)
	{
		return MaxPerInput == 1 ? FPinConnectionResponse(Input == A ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("ReplaceInput", "Replace the existing link.")) : FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InputFull", "This input already has all the links it takes."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UKzEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bModified = Super::TryCreateConnection(A, B);
	if (bModified)
	{
		NotifySoon(A->GetOwningNode()->GetGraph());
	}
	return bModified;
}

void UKzEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
	NotifySoon(TargetPin.GetOwningNode()->GetGraph());
}

void UKzEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	Super::BreakSinglePinLink(SourcePin, TargetPin);
	NotifySoon(SourcePin->GetOwningNode()->GetGraph());
}

void UKzEdGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2f& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("RerouteWire", "Create Reroute Node"));
	UEdGraph* Graph = PinA->GetOwningNode()->GetGraph();
	Graph->Modify();

	// Centered on the click: the dot's widget is 42 by 24.
	UKzEdGraphNode_Reroute* Reroute = UKzEdGraphNode_Reroute::Create(*Graph, GraphPosition - FVector2f(21.0f, 12.0f), true);
	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo(PinA->Direction == EGPD_Output ? Reroute->Pins[0] : Reroute->Pins[1]);
	PinB->MakeLinkTo(PinB->Direction == EGPD_Output ? Reroute->Pins[0] : Reroute->Pins[1]);
	Graph->NotifyGraphChanged();
}

FConnectionDrawingPolicy* UKzEdGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FKzGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, KzGraphEditor::GetGraphAsset(InGraphObj));
}

TSharedPtr<FEdGraphSchemaAction> UKzEdGraphSchema::GetCreateCommentAction() const
{
	return MakeShared<FKzGraphSchemaAction_Comment>(FText::GetEmpty(), LOCTEXT("AddComment", "Add Comment"), LOCTEXT("AddCommentTooltip", "Adds a comment box around the selected nodes, or an empty one."), 0);
}

void UKzEdGraphSchema::ForEachNodeStruct(const UKzGraph& Graph, TFunctionRef<void(const UScriptStruct*)> Callback)
{
	const UScriptStruct* Base = Graph.GetNodeStruct();
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		const UScriptStruct* NodeStruct = *It;
		if (NodeStruct != Base && NodeStruct->IsChildOf(Base) && !NodeStruct->HasMetaData(TEXT("Hidden")))
		{
			Callback(NodeStruct);
		}
	}
}

const UScriptStruct* UKzEdGraphSchema::FindNodeStructForShortcut(const UKzGraph& Graph, const FKey& Key)
{
	const FString KeyName = Key.GetFName().ToString();
	const UScriptStruct* Found = nullptr;
	ForEachNodeStruct(Graph, [&KeyName, &Found](const UScriptStruct* NodeStruct)
	{
		if (!Found && NodeStruct->GetMetaData(TEXT("Shortcut")) == KeyName)
		{
			Found = NodeStruct;
		}
	});
	return Found;
}

UKzEdGraphNode* UKzEdGraphSchema::CreateNode(UEdGraph& Graph, const UScriptStruct* NodeStruct, const FVector2f& Location, bool bSelect)
{
	TInstancedStruct<FKzGraphNode> Node;
	Node.InitializeAsScriptStruct(NodeStruct);
	return CreateNode(Graph, Node, Location, bSelect);
}

UKzEdGraphNode* UKzEdGraphSchema::CreateNode(UEdGraph& Graph, const TInstancedStruct<FKzGraphNode>& Node, const FVector2f& Location, bool bSelect)
{
	FGraphNodeCreator<UKzEdGraphNode> Creator(Graph);
	UKzEdGraphNode* EdNode = Creator.CreateNode(bSelect);
	EdNode->Node = Node;
	EdNode->NodePosX = static_cast<int32>(Location.X);
	EdNode->NodePosY = static_cast<int32>(Location.Y);
	Creator.Finalize();
	return EdNode;
}

void UKzEdGraphSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2f& GraphPosition, UEdGraph* Graph) const
{
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(Graph);
	if (!Asset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DropAssets", "Add Nodes From Assets"));
	Graph->Modify();

	// Several assets land in a column, one grid cell apart.
	FVector2f Location = GraphPosition;
	for (const FAssetData& Dropped : Assets)
	{
		UObject* Object = Dropped.GetAsset();
		TInstancedStruct<FKzGraphNode> Node;
		if (Object && Asset->MakeNodeForAsset(*Object, Node) && Node.IsValid())
		{
			CreateNode(*Graph, Node, Location, true);
			Location.Y += 8.0f * SNodePanel::GetSnapGridSize();
		}
	}
	Graph->NotifyGraphChanged();
}

void UKzEdGraphSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	OutOkIcon = false;
	const UKzGraph* Asset = KzGraphEditor::GetGraphAsset(HoverGraph);
	for (const FAssetData& Hovered : Assets)
	{
		// The graph is asked with the asset loaded, as the drop loads it anyway.
		UObject* Object = Asset ? Hovered.GetAsset() : nullptr;
		TInstancedStruct<FKzGraphNode> Node;
		if (Object && Asset->MakeNodeForAsset(*Object, Node) && Node.IsValid())
		{
			OutOkIcon = true;
			OutTooltipText = FText::Format(LOCTEXT("DropAssetsTip", "Add a {0} node"), Node.GetScriptStruct()->GetDisplayNameText()).ToString();
			return;
		}
	}
	OutTooltipText = LOCTEXT("DropAssetsNone", "This graph makes no node from these assets").ToString();
}

void UKzEdGraphSchema::NotifySoon(UEdGraph* Graph)
{
	if (UKzEdGraph* KzGraph = Cast<UKzEdGraph>(Graph))
	{
		KzGraph->NotifyGraphChangedSoon();
	}
	else if (Graph)
	{
		Graph->NotifyGraphChanged();
	}
}

UEdGraphNode* FKzGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
{
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UKzEdGraphNode* Node = UKzEdGraphSchema::CreateNode(*ParentGraph, NodeStruct, Location, bSelectNewNode);
	Node->AutowireNewNode(FromPin);
	ParentGraph->NotifyGraphChanged();
	return Node;
}

UEdGraphNode* FKzGraphSchemaAction_Comment::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
{
	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
	FVector2f SpawnLocation = Location;

	FSlateRect Bounds;
	const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(ParentGraph);
	if (GraphEditor.IsValid() && GraphEditor->GetBoundsForSelectedNodes(Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation = FVector2f(static_cast<float>(CommentTemplate->NodePosX), static_cast<float>(CommentTemplate->NodePosY));
	}

	return FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);
}

UEdGraphNode* FKzGraphSchemaAction_Reroute::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
{
	const FScopedTransaction Transaction(LOCTEXT("AddRerouteTransaction", "Add Reroute Node"));
	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}

	UKzEdGraphNode_Reroute* Reroute = UKzEdGraphNode_Reroute::Create(*ParentGraph, Location, bSelectNewNode);
	Reroute->AutowireNewNode(FromPin);
	ParentGraph->NotifyGraphChanged();
	return Reroute;
}

#undef LOCTEXT_NAMESPACE
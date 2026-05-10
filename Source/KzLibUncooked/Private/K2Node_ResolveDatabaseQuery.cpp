// Copyright 2026 kirzo

#include "K2Node_ResolveDatabaseQuery.h"
#include "Kismet/KzDatabaseLibrary.h"
#include "Components/KzDatabaseComponent.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_ResolveDatabaseQuery"

void UK2Node_ResolveDatabaseQuery::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Execs
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	// Target Component
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UKzDatabaseComponent::StaticClass(), TEXT("Component"));

	// Query
	UScriptStruct* QueryStruct = FKzDatabaseQuery::StaticStruct();
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, QueryStruct, TEXT("Query"));

	// Outputs
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("bSuccess"));

	// The Wildcard Result Pin (Starts as undetermined)
	UEdGraphNode::FCreatePinParams PinParams;
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, TEXT("Result"), PinParams);
}

void UK2Node_ResolveDatabaseQuery::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// If the user connects or disconnects the Result pin...
	if (Pin && Pin->PinName == TEXT("Result"))
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			// Match the exact type of the pin we just connected to
			Pin->PinType = Pin->LinkedTo[0]->PinType;
		}
		else
		{
			// The user broke the link. Revert back to grey wildcard so they can choose a new type.
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			Pin->PinType.PinSubCategory = NAME_None;
			Pin->PinType.PinSubCategoryObject = nullptr;
			Pin->PinType.ContainerType = EPinContainerType::None;
		}

		// Force the node to visually refresh its color
		GetGraph()->NotifyGraphChanged();
	}
}

void UK2Node_ResolveDatabaseQuery::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* ResultPin = FindPinChecked(TEXT("Result"));

	// Prevent compilation if the pin is still grey (not connected)
	if (ResultPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("UndeterminedTypeError", "The type of @@ is undetermined. Connect something to imply a specific type.").ToString(), ResultPin);
		BreakAllNodeLinks();
		return;
	}

	// 1. Manually spawn the intermediate node to avoid LNK2019
	UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(SourceGraph);
	CallFuncNode->SetFlags(RF_Transactional);
	SourceGraph->AddNode(CallFuncNode);
	CallFuncNode->CreateNewGuid();
	CallFuncNode->PostPlacedNewNode();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFuncNode, this);

	CallFuncNode->SetFromFunction(UKzDatabaseLibrary::StaticClass()->FindFunctionByName(GET_MEMBER_NAME_CHECKED(UKzDatabaseLibrary, ResolveDatabaseQuery)));
	CallFuncNode->AllocateDefaultPins();

	const UEdGraphSchema* Schema = CompilerContext.GetSchema();
	auto MoveLinks = [Schema](UEdGraphPin* Source, UEdGraphPin* Dest)
		{
			if (Source && Dest) Schema->MovePinLinks(*Source, *Dest, true);
		};

	// 2. Map standard pins
	MoveLinks(GetExecPin(), CallFuncNode->GetExecPin());
	MoveLinks(FindPinChecked(UEdGraphSchema_K2::PN_Execute, EGPD_Output), CallFuncNode->GetThenPin());
	MoveLinks(FindPinChecked(TEXT("Component")), CallFuncNode->FindPinChecked(TEXT("Component")));
	MoveLinks(FindPinChecked(TEXT("Query")), CallFuncNode->FindPinChecked(TEXT("Query")));
	MoveLinks(FindPinChecked(TEXT("bSuccess")), CallFuncNode->GetReturnValuePin());

	// 3. Map the magic typed pin
	UEdGraphPin* InternalOutPin = CallFuncNode->FindPinChecked(TEXT("OutValue"));
	InternalOutPin->PinType = ResultPin->PinType;
	MoveLinks(ResultPin, InternalOutPin);

	BreakAllNodeLinks();
}

FText UK2Node_ResolveDatabaseQuery::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("ResolveDB_Title", "Resolve Database Query");
}

FText UK2Node_ResolveDatabaseQuery::GetTooltipText() const
{
	return LOCTEXT("ResolveDB_Tooltip", "Asks the target Database Component to resolve a query. Connect the Result pin to infer the data type.");
}

void UK2Node_ResolveDatabaseQuery::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ResolveDatabaseQuery::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
}

#undef LOCTEXT_NAMESPACE
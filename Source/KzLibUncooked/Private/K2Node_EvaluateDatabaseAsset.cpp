// Copyright 2026 kirzo

#include "K2Node_EvaluateDatabaseAsset.h"
#include "KzParamDefEditorUtils.h"
#include "Kismet/KzDatabaseLibrary.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_EvaluateDatabaseAsset"

UK2Node_EvaluateDatabaseAsset::UK2Node_EvaluateDatabaseAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UK2Node_EvaluateDatabaseAsset::PreloadRequiredAssets()
{
	if (DatabaseAsset)
	{
		PreloadObject(DatabaseAsset);
	}

	Super::PreloadRequiredAssets();
}

void UK2Node_EvaluateDatabaseAsset::EarlyValidation(FCompilerResultsLog& MessageLog) const
{
	Super::EarlyValidation(MessageLog);

	if (!DatabaseAsset)
	{
		MessageLog.Error(*LOCTEXT("MissingAssetError", "Evaluate Database Node @@ requires a valid Database Asset.").ToString(), this);
		return;
	}

	const UEdGraphPin* ResultPin = FindPin(TEXT("Result"));
	const bool bAssetHasValidType = DatabaseAsset->GetDataType().IsValid();

	if (ResultPin && bAssetHasValidType)
	{
		// 1. We have a pin and the asset has a type. Let's compare them.
		FEdGraphPinType ExpectedPinType = KzLib::Editor::PinTypeFromDef(DatabaseAsset->GetDataType());

		if (ResultPin->PinType != ExpectedPinType)
		{
			MessageLog.Error(*LOCTEXT("PinTypeMismatch", "The data type in the Database Asset for @@ has changed. Please right-click the node and select 'Refresh Node'.").ToString(), this);
		}
	}
	else if (!ResultPin && bAssetHasValidType)
	{
		// 2. The node has no pin, but the asset got a valid type assigned externally.
		MessageLog.Error(*LOCTEXT("PinMissing", "The Database Asset for @@ now has a valid type, but the node doesn't reflect it. Please right-click and 'Refresh Node'.").ToString(), this);
	}
	else if (ResultPin && !bAssetHasValidType)
	{
		// 3. The node has a pin, but the asset's type was removed/invalidated externally.
		MessageLog.Error(*LOCTEXT("AssetInvalidated", "The Database Asset for @@ is no longer valid. Please right-click and 'Refresh Node'.").ToString(), this);
	}
}

void UK2Node_EvaluateDatabaseAsset::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

	UScriptStruct* QueryStruct = FKzDatabaseQuery::StaticStruct();
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, QueryStruct, TEXT("Query"));

	UEdGraphPin* SuccessPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("bSuccess"));

	if (DatabaseAsset && DatabaseAsset->GetDataType().IsValid())
	{
		UEdGraphNode::FCreatePinParams PinParams;
		UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, TEXT("Result"), PinParams);

		ResultPin->PinType = KzLib::Editor::PinTypeFromDef(DatabaseAsset->GetDataType());
	}
}

void UK2Node_EvaluateDatabaseAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_EvaluateDatabaseAsset, DatabaseAsset))
	{
		// Force the node to recreate its pins when the user selects a new Database Asset
		ReconstructNode();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_EvaluateDatabaseAsset::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (!DatabaseAsset)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingAssetError", "Evaluate Database Node @@ requires a valid Database Asset.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	// 1. Manually spawn the intermediate node to avoid LNK2019 (bypassing unexported SpawnIntermediateNode)
	UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(SourceGraph);
	CallFuncNode->SetFlags(RF_Transactional);
	SourceGraph->AddNode(CallFuncNode);
	CallFuncNode->CreateNewGuid();
	CallFuncNode->PostPlacedNewNode();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFuncNode, this);

	// Set up the actual function call
	CallFuncNode->SetFromFunction(UKzDatabaseLibrary::StaticClass()->FindFunctionByName(GET_MEMBER_NAME_CHECKED(UKzDatabaseLibrary, EvaluateDatabaseAsset)));
	CallFuncNode->AllocateDefaultPins();

	// 2. Pass the selected DatabaseAsset directly to the hidden function
	UEdGraphPin* FuncAssetPin = CallFuncNode->FindPinChecked(TEXT("Asset"));
	CallFuncNode->GetSchema()->TrySetDefaultObject(*FuncAssetPin, DatabaseAsset);

	// Helper to move pins safely using the Schema directly (bypassing unexported MovePinLinksToIntermediate)
	const UEdGraphSchema* Schema = CompilerContext.GetSchema();
	auto MoveLinks = [Schema](UEdGraphPin* Source, UEdGraphPin* Dest)
		{
			if (Source && Dest)
			{
				Schema->MovePinLinks(*Source, *Dest, true);
			}
		};

	// 3. Move connections from our visual node to the hidden CallFunction node
	MoveLinks(GetExecPin(), CallFuncNode->GetExecPin());
	MoveLinks(FindPinChecked(UEdGraphSchema_K2::PN_Execute, EGPD_Output), CallFuncNode->GetThenPin());

	MoveLinks(FindPinChecked(TEXT("Query")), CallFuncNode->FindPinChecked(TEXT("Query")));
	MoveLinks(FindPinChecked(TEXT("bSuccess")), CallFuncNode->GetReturnValuePin());

	// The magic connection: Map our strongly-typed Result pin to the Custom Thunk's Wildcard out pin
	if (UEdGraphPin* ResultPin = FindPin(TEXT("Result")))
	{
		UEdGraphPin* InternalOutPin = CallFuncNode->FindPinChecked(TEXT("OutValue"));
		InternalOutPin->PinType = ResultPin->PinType;
		MoveLinks(ResultPin, InternalOutPin);
	}

	// Clean up our visual node
	BreakAllNodeLinks();
}

FText UK2Node_EvaluateDatabaseAsset::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (DatabaseAsset)
	{
		return FText::Format(LOCTEXT("EvaluateDB_TitleWithAsset", "Evaluate Database: {0}"), FText::FromString(DatabaseAsset->GetName()));
	}
	return LOCTEXT("EvaluateDB_Title", "Evaluate Database");
}

FText UK2Node_EvaluateDatabaseAsset::GetTooltipText() const
{
	return LOCTEXT("EvaluateDB_Tooltip", "Evaluates a Database Asset using a Query and returns the best matching result.");
}

void UK2Node_EvaluateDatabaseAsset::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_EvaluateDatabaseAsset::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "K2Node_EvaluateDatabaseAsset.h"
#include "KzPinTypeUtils.h"
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

	const FKzTypeDef& TypeDef = DatabaseAsset->GetDataType();

	const UEdGraphPin* ResultPin = FindPin(TEXT("Result"));
	const bool bAssetHasValidType = TypeDef.IsValid();

	if (ResultPin && bAssetHasValidType)
	{
		// 1. We have a pin and the asset has a type. Let's compare them.
		FEdGraphPinType ExpectedPinType = KzLib::Editor::PinTypeFromBagType(TypeDef.ValueType, TypeDef.ValueTypeObject.Get(), TypeDef.ContainerType);
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

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("Found"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("NotFound"));

	UScriptStruct* QueryStruct = FKzDatabaseQuery::StaticStruct();
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, QueryStruct, TEXT("Query"));

	if (DatabaseAsset && DatabaseAsset->GetDataType().IsValid())
	{
		UEdGraphNode::FCreatePinParams PinParams;
		UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, TEXT("Result"), PinParams);

		const FKzTypeDef& TypeDef = DatabaseAsset->GetDataType();
		ResultPin->PinType = KzLib::Editor::PinTypeFromBagType(TypeDef.ValueType, TypeDef.ValueTypeObject.Get(), TypeDef.ContainerType);
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

	UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(SourceGraph);
	CallFuncNode->SetFlags(RF_Transactional);
	SourceGraph->AddNode(CallFuncNode);
	CallFuncNode->CreateNewGuid();
	CallFuncNode->PostPlacedNewNode();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFuncNode, this);

	CallFuncNode->SetFromFunction(UKzDatabaseLibrary::StaticClass()->FindFunctionByName(GET_MEMBER_NAME_CHECKED(UKzDatabaseLibrary, EvaluateDatabaseAsset)));
	CallFuncNode->AllocateDefaultPins();

	UEdGraphPin* FuncAssetPin = CallFuncNode->FindPinChecked(TEXT("Asset"));
	CallFuncNode->GetSchema()->TrySetDefaultObject(*FuncAssetPin, DatabaseAsset);

	const UEdGraphSchema* Schema = CompilerContext.GetSchema();
	auto MoveLinks = [Schema](UEdGraphPin* Source, UEdGraphPin* Dest)
		{
			if (Source && Dest)
			{
				Schema->MovePinLinks(*Source, *Dest, true);
			}
		};

	MoveLinks(GetExecPin(), CallFuncNode->GetExecPin());

	// ExpandEnumAsExecs generates one exec pin per enum value, named after the value.
	MoveLinks(FindPinChecked(TEXT("Found")), CallFuncNode->FindPinChecked(TEXT("Found")));
	MoveLinks(FindPinChecked(TEXT("NotFound")), CallFuncNode->FindPinChecked(TEXT("NotFound")));

	MoveLinks(FindPinChecked(TEXT("Query")), CallFuncNode->FindPinChecked(TEXT("Query")));

	if (UEdGraphPin* ResultPin = FindPin(TEXT("Result")))
	{
		UEdGraphPin* InternalOutPin = CallFuncNode->FindPinChecked(TEXT("OutValue"));
		InternalOutPin->PinType = ResultPin->PinType;
		MoveLinks(ResultPin, InternalOutPin);
	}

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
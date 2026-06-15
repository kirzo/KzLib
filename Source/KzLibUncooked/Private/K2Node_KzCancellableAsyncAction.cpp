// Copyright 2026 kirzo

#include "K2Node_KzCancellableAsyncAction.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "K2Node_KzCancellableAsyncAction"

const FName UK2Node_KzCancellableAsyncAction::CancelInputPinName(TEXT("CancelAsyncExec"));
const FName UK2Node_KzCancellableAsyncAction::CancelFunctionName(TEXT("Cancel"));

void UK2Node_KzCancellableAsyncAction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* CancelPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, CancelInputPinName);
	CancelPin->PinFriendlyName = LOCTEXT("CancelPinFriendlyName", "Cancel");
	CancelPin->PinToolTip = TEXT("Aborts the action: calls Cancel() on it, which stops it and routes execution out of the Cancelled pin.");

	// Move Cancel up next to the main exec input (exec -> Cancel -> data inputs).
	if (UEdGraphPin* MainExecPin = FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input))
	{
		Pins.Remove(CancelPin);
		Pins.Insert(CancelPin, Pins.IndexOfByKey(MainExecPin) + 1);
	}

	// Push the exposed proxy pin last (after the exec outputs). Safe: the base ExpandNode finds it by name.
	if (UEdGraphPin* ProxyPin = FindPin(FBaseAsyncTaskHelper::GetAsyncTaskProxyName(), EGPD_Output))
	{
		Pins.Remove(ProxyPin);
		Pins.Add(ProxyPin);
	}
}

FSlateIcon UK2Node_KzCancellableAsyncAction::GetIconAndTint(FLinearColor& OutColor) const
{
	// Blue "f" function icon. Color is FunctionCallNodeTitleColor's default, hardcoded to avoid a
	// GraphEditor dependency from this UncookedOnly module.
	OutColor = FLinearColor(0.190525f, 0.583898f, 1.0f, 1.0f);
	static const FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "Kismet.AllClasses.FunctionIcon");
	return Icon;
}

void UK2Node_KzCancellableAsyncAction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	struct FUtils
	{
		static void SetNodeFunc(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UFunction> FunctionPtr)
		{
			UK2Node_KzCancellableAsyncAction* Node = CastChecked<UK2Node_KzCancellableAsyncAction>(NewNode);
			if (FunctionPtr.IsValid())
			{
				Node->InitializeProxyFromFunction(FunctionPtr.Get());
			}
		}
	};

	// Not Super: the base would claim every async action. Claim only the opted-out (HasDedicatedAsyncNode)
	// factories whose proxy has a Cancel function.
	UClass* NodeClass = GetClass();
	ActionRegistrar.RegisterClassFactoryActions<UBlueprintAsyncActionBase>(FBlueprintActionDatabaseRegistrar::FMakeFuncSpawnerDelegate::CreateLambda(
		[NodeClass](const UFunction* FactoryFunc) -> UBlueprintNodeSpawner*
		{
			const UClass* FactoryClass = FactoryFunc ? FactoryFunc->GetOwnerClass() : nullptr;
			if (!FactoryClass || !FactoryClass->HasMetaData(TEXT("HasDedicatedAsyncNode")))
			{
				return nullptr;
			}

			const FObjectProperty* ReturnProp = CastField<FObjectProperty>(FactoryFunc->GetReturnProperty());
			const UClass* ProxyClass = ReturnProp ? ReturnProp->PropertyClass : nullptr;
			if (!ProxyClass || !ProxyClass->FindFunctionByName(UK2Node_KzCancellableAsyncAction::CancelFunctionName))
			{
				return nullptr;
			}

			UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunc);
			check(NodeSpawner != nullptr);
			NodeSpawner->NodeClass = NodeClass;

			TWeakObjectPtr<UFunction> FunctionPtr = MakeWeakObjectPtr(const_cast<UFunction*>(FactoryFunc));
			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(FUtils::SetNodeFunc, FunctionPtr);
			return NodeSpawner;
		}));
}

bool UK2Node_KzCancellableAsyncAction::HandleDelegates(
	const TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable>& VariableOutputs, UEdGraphPin* ProxyObjectPin,
	UEdGraphPin*& InOutLastThenPin, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
{
	bool bIsErrorFree = Super::HandleDelegates(VariableOutputs, ProxyObjectPin, InOutLastThenPin, SourceGraph, CompilerContext);

	// Route the Cancel input exec to a Cancel() call on the proxy. Separate entry path: don't touch InOutLastThenPin.
	UEdGraphPin* CancelInputPin = FindPin(CancelInputPinName, EGPD_Input);
	UFunction* CancelFunc = ProxyClass ? ProxyClass->FindFunctionByName(CancelFunctionName) : nullptr;
	if (CancelInputPin && CancelInputPin->LinkedTo.Num() > 0 && CancelFunc)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Build the call node by hand (not CompilerContext.SpawnIntermediateNode / MovePinLinksToIntermediate)
		// to avoid linking the KismetCompiler module from this UncookedOnly module.
		UK2Node_CallFunction* CallCancelNode = NewObject<UK2Node_CallFunction>(SourceGraph);
		CallCancelNode->SetFlags(RF_Transactional);
		SourceGraph->AddNode(CallCancelNode);
		CallCancelNode->CreateNewGuid();
		CallCancelNode->PostPlacedNewNode();
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallCancelNode, this);

		CallCancelNode->SetFromFunction(CancelFunc);
		CallCancelNode->AllocateDefaultPins();

		// Self <- the proxy (compiler caches it in a local; null if the action never started).
		UEdGraphPin* CallSelfPin = CallCancelNode->FindPinChecked(UEdGraphSchema_K2::PN_Self, EGPD_Input);
		bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, CallSelfPin);

		bIsErrorFree &= Schema->MovePinLinks(*CancelInputPin, *CallCancelNode->GetExecPin(), /*bIsIntermediateMove=*/true).CanSafeConnect();
	}

	return bIsErrorFree;
}

#undef LOCTEXT_NAMESPACE

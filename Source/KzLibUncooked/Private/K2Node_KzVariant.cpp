// Copyright 2026 kirzo

#include "K2Node_KzVariant.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Core/KzVariant.h"
#include "Kismet/KzVariantLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_KzVariant)

#define LOCTEXT_NAMESPACE "KzVariant"

void UK2Node_KzVariant::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();
	if (!ActionRegistrar.IsOpenForRegistration(Action)) { return; }

	const FName FunctionNames[] = {
		GET_FUNCTION_NAME_CHECKED(UKzVariantLibrary, MakeKzVariant),
		GET_FUNCTION_NAME_CHECKED(UKzVariantLibrary, BreakKzVariant),
		GET_FUNCTION_NAME_CHECKED(UKzVariantLibrary, GetKzVariant),
		GET_FUNCTION_NAME_CHECKED(UKzVariantLibrary, SetKzVariant),
	};

	for (const FName& FunctionName : FunctionNames)
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner);

		Spawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(
			[FunctionName](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/)
			{
				UK2Node_KzVariant* Node = CastChecked<UK2Node_KzVariant>(NewNode);
				UFunction* Function = UKzVariantLibrary::StaticClass()->FindFunctionByName(FunctionName);
				check(Function);
				Node->SetFromFunction(Function);
			});

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

bool UK2Node_KzVariant::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	const UEdGraphPin* ValuePin = FindPin(FName(TEXT("Value")));

	if (ValuePin && MyPin == ValuePin && MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		if (UEdGraphSchema_K2::IsExecPin(*OtherPin))
		{
			OutReason = TEXT("Value can't be Exec.");
			return true;
		}
		if (OtherPin->PinType.IsContainer())
		{
			OutReason = TEXT("KzVariant doesn't support arrays, sets or maps.");
			return true;
		}
	}

	return false;
}

#undef CREATE_NODE_SPAWNER
#undef LOCTEXT_NAMESPACE
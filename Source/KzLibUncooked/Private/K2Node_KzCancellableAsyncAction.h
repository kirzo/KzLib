// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_KzCancellableAsyncAction.generated.h"

/**
 * Async-action node variant with a second input exec pin, "Cancel", wired to the proxy object's
 * Cancel() function -- so a latent action can be aborted straight from the graph, without storing the
 * node's return value in a variable.
 *
 * It auto-applies to any UBlueprintAsyncActionBase whose factory class is marked
 * meta=(HasDedicatedAsyncNode) (which opts it out of the engine's default async node) and whose proxy
 * class exposes a callable Cancel() function. There is no per-node code: a new cancellable async node
 * only needs those two things.
 */
UCLASS()
class UK2Node_KzCancellableAsyncAction : public UK2Node_AsyncAction
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	//~ End UEdGraphNode interface

	//~ Begin UK2Node interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UK2Node interface

protected:
	//~ Begin UK2Node_BaseAsyncTask interface
	virtual bool HandleDelegates(
		const TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable>& VariableOutputs, UEdGraphPin* ProxyObjectPin,
		UEdGraphPin*& InOutLastThenPin, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext) override;
	//~ End UK2Node_BaseAsyncTask interface

private:
	/** Internal name of the extra "Cancel" input exec pin. */
	static const FName CancelInputPinName;

	/** Proxy function the Cancel pin routes to. */
	static const FName CancelFunctionName;
};

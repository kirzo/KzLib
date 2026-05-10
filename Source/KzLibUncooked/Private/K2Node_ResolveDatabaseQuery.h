// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_ResolveDatabaseQuery.generated.h"

/**
 * A node that asks a KzDatabaseComponent to resolve a query dynamically.
 * The output type is inferred by what the user connects to the Result pin.
 */
UCLASS(MinimalAPI)
class UK2Node_ResolveDatabaseQuery : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// Magic happens here: We listen to pin connection changes!
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual bool IsNodePure() const override { return false; }
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface
};
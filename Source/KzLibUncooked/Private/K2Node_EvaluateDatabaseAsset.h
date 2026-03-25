// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Core/KzDatabaseAsset.h"
#include "K2Node_EvaluateDatabaseAsset.generated.h"

/**
 * A custom Blueprint node that evaluates a UKzDatabaseAsset.
 * The output pin dynamically changes its type based on the FKzParamDef configured inside the selected asset.
 */
UCLASS(MinimalAPI)
class UK2Node_EvaluateDatabaseAsset : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_EvaluateDatabaseAsset(const FObjectInitializer& ObjectInitializer);

	// The Database Asset configured directly on the node
	UPROPERTY(EditAnywhere, Category = "Database")
	TObjectPtr<UKzDatabaseAsset> DatabaseAsset;

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface.
	virtual bool IsNodePure() const override { return false; }
	virtual void PreloadRequiredAssets() override;
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface.
};
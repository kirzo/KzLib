// Copyright 2026 kirzo

#include "KzGraphFactory.h"
#include "KzEdGraph.h"
#include "KzGraph.h"

UKzGraphFactory::UKzGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UKzGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UKzGraph* Graph = NewObject<UKzGraph>(InParent, Class, Name, Flags | RF_Transactional);
	UKzEdGraph::CreateFor(*Graph);
	return Graph;
}
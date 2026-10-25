// Copyright 2026 kirzo

#include "KzGraph.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, KzGraph)

int32 FKzGraphData::FindEntry(FName Name) const
{
	const FKzGraphEntry* Entry = Entries.FindByPredicate([Name](const FKzGraphEntry& Each) { return Each.Name == Name; });
	return Entry ? Entry->Node : INDEX_NONE;
}

int32 FKzGraphData::FindOutput(int32 Node, FName Name) const
{
	const FKzGraphNodeRanges& Range = Ranges[Node];
	for (int32 Output = 0; Output < Range.NumOutputs; ++Output)
	{
		if (Outputs[Range.FirstOutput + Output].Name == Name)
		{
			return Output;
		}
	}
	return INDEX_NONE;
}

TConstArrayView<FKzGraphLink> FKzGraphData::GetLinks(int32 Node, int32 Output) const
{
	const FKzGraphOutput& Entry = Outputs[Ranges[Node].FirstOutput + Output];
	return TConstArrayView<FKzGraphLink>(Links.GetData() + Entry.FirstLink, Entry.NumLinks);
}

#if WITH_EDITOR
void FKzGraphNode::GetInputPins(TArray<FKzGraphPinDesc>& OutPins) const
{
	OutPins.Add({ TEXT("In") });
}

void FKzGraphNode::GetOutputPins(TArray<FKzGraphPinDesc>& OutPins) const
{
	OutPins.Add({ TEXT("Out") });
}
#endif
// Copyright 2026 kirzo

#include "Containers/KzShuffleBag.h"
#include "Algo/Compare.h"

namespace
{
	int32 RandRange(FRandomStream* Stream, int32 Min, int32 Max)
	{
		return Stream ? Stream->RandRange(Min, Max) : FMath::RandRange(Min, Max);
	}
}

//----------------------------------------------------------------------//
//  FKzShuffleRound
//----------------------------------------------------------------------//

int32 FKzShuffleRound::Draw(TConstArrayView<int32> Weights, bool bNoRepeat, FRandomStream* Stream)
{
	if (!Algo::Compare(DealtWeights, Weights))
	{
		DealtWeights.Reset();
		DealtWeights.Append(Weights.GetData(), Weights.Num());
		Remaining.Reset();
	}

	if (Remaining.IsEmpty())
	{
		Deal(bNoRepeat, Stream);
		if (Remaining.IsEmpty())
		{
			return INDEX_NONE;
		}
	}

	Last = Remaining.Pop(EAllowShrinking::No);
	return Last;
}

void FKzShuffleRound::Deal(bool bNoRepeat, FRandomStream* Stream)
{
	for (int32 Index = 0; Index < DealtWeights.Num(); ++Index)
	{
		for (int32 Copy = 0; Copy < DealtWeights[Index]; ++Copy)
		{
			Remaining.Add(Index);
		}
	}

	for (int32 Index = Remaining.Num() - 1; Index > 0; --Index)
	{
		Remaining.Swap(Index, RandRange(Stream, 0, Index));
	}

	// The back opens the round. Only for entries that are at most half of it: moving a frequent one would make the others open most rounds
	const int32 Opening = Remaining.Num() - 1;
	if (!bNoRepeat || Opening < 1 || Remaining[Opening] != Last || DealtWeights[Last] * 2 > Remaining.Num())
	{
		return;
	}

	int32 Pick = RandRange(Stream, 0, Remaining.Num() - DealtWeights[Last] - 1);
	for (int32 Index = 0; Index < Opening; ++Index)
	{
		if (Remaining[Index] != Last && Pick-- == 0)
		{
			Remaining.Swap(Index, Opening);
			return;
		}
	}
}

//----------------------------------------------------------------------//
//  FKzShuffleBag
//----------------------------------------------------------------------//

int32 FKzShuffleBag::Draw(FRandomStream* Stream)
{
	TArray<int32, TInlineAllocator<16>> Weights;
	for (const FKzShuffleBagEntry& Entry : Entries)
	{
		Weights.Add(FMath::Max(Entry.Weight, 0));
	}
	return Round.Draw(Weights, bNoRepeatAcrossRounds, Stream);
}

bool FKzShuffleBag::Add(FKzVariant Value, int32 Weight)
{
	if (!Value.IsValid())
	{
		return false;
	}

	if (!Type.IsValid())
	{
		if (!Entries.IsEmpty())
		{
			return false;
		}
		Type = FKzTypeDef(Value.GetType(), Value.GetTypeObject());
	}
	else if (!Value.MatchesType(Type))
	{
		return false;
	}

	FKzShuffleBagEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Value = MoveTemp(Value);
	Entry.Weight = Weight;
	return true;
}

bool FKzShuffleBag::AddWeight(int32 Weight)
{
	if (Type.IsValid())
	{
		return false;
	}

	Entries.AddDefaulted_GetRef().Weight = Weight;
	return true;
}

void FKzShuffleBag::Empty()
{
	Entries.Empty();
	Round.Reset();
}

void FKzShuffleBag::SyncTypes()
{
	for (FKzShuffleBagEntry& Entry : Entries)
	{
		if (!Type.IsValid())
		{
			Entry.Value.Reset();
		}
		else if (!Entry.Value.MatchesType(Type))
		{
			Entry.Value.SetType(Type.ValueType, Type.ValueTypeObject);
		}
	}
}
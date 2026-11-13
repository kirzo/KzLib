// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzVariant.h"
#include "Math/RandomStream.h"
#include "KzShuffleBag.generated.h"

/** The round of a shuffle bag: what is left to draw, the weights it was dealt with and the last draw. */
USTRUCT()
struct KZLIB_API FKzShuffleRound
{
	GENERATED_BODY()

	/**
	 * An index into Weights: each comes out as many times as its weight a round, in a new order each round. INDEX_NONE when none has
	 * weight. Weights other than the round's deal a new one. With bNoRepeat, an entry that is at most half of a round doesn't open a round
	 * it closed the last one with.
	 */
	int32 Draw(TConstArrayView<int32> Weights, bool bNoRepeat, FRandomStream* Stream = nullptr);

	/** The next draw deals a new round. */
	void Reset() { Remaining.Reset(); }

private:
	void Deal(bool bNoRepeat, FRandomStream* Stream);

	/** Drawn from the back. */
	UPROPERTY()
	TArray<int32> Remaining;

	UPROPERTY()
	TArray<int32> DealtWeights;

	UPROPERTY()
	int32 Last = INDEX_NONE;
};

/** A value in a shuffle bag and how many times it comes out a round. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzShuffleBagEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Shuffle Bag", meta = (FixedType, ShowOnlyInnerProperties))
	FKzVariant Value;

	/** Copies of the value a round: with 1 and 4, the first comes out once every five draws. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shuffle Bag", meta = (ClampMin = "0"))
	int32 Weight = 1;
};

/**
 * A shuffle bag: every entry comes out as many times as its weight before any comes out again, in a new order each round, so a value
 * neither goes long without coming out nor keeps coming out by chance. Its entries hold values of its type; without a type, they are
 * weights alone, drawn by index. The round in course is not saved.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzShuffleBag
{
	GENERATED_BODY()

	FKzShuffleBag()
	{
		Type.ValueType = EPropertyBagPropertyType::None;
	}

	/** The type of its values; none for a bag of weights alone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shuffle Bag", meta = (NoArrays))
	FKzTypeDef Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shuffle Bag")
	TArray<FKzShuffleBagEntry> Entries;

	/** A new round doesn't open with the entry the last one closed with, for entries that are at most half of a round. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shuffle Bag")
	bool bNoRepeatAcrossRounds = true;

	/** The next entry's index, INDEX_NONE when none has weight. A change of weights deals a new round. */
	int32 Draw(FRandomStream* Stream = nullptr);

	/** The next value; false when none has weight or the bag doesn't hold T. */
	template <typename T>
	bool Draw(T& OutValue, FRandomStream* Stream = nullptr)
	{
		const int32 Index = Draw(Stream);
		if (Index == INDEX_NONE || !Entries[Index].Value.Is<T>())
		{
			return false;
		}
		OutValue = Entries[Index].Value.Get<T>();
		return true;
	}

	/** Adds a value of the bag's type; a bag without a type nor entries takes the value's. */
	template <typename T>
	bool Add(const T& Value, int32 Weight = 1)
	{
		return Add(FKzVariant::Make<T>(Value), Weight);
	}

	bool Add(FKzVariant Value, int32 Weight = 1);

	/** Adds an entry without a value, to a bag without a type. */
	bool AddWeight(int32 Weight);

	/** The next draw deals a new round. */
	void Reset() { Round.Reset(); }

	void Empty();

	/** Gives every entry a value of the bag's type, keeping the ones that have it already; none without a type. */
	void SyncTypes();

private:
	UPROPERTY(Transient)
	FKzShuffleRound Round;
};

namespace Kz
{
	/** A shuffle bag of any C++ type, for values FKzShuffleBag can't hold or that Blueprint and the editor never see. */
	template <typename T>
	class TShuffleBag
	{
	public:
		explicit TShuffleBag(int32 Seed = 0) : Stream(Seed != 0 ? Seed : FMath::Rand()) {}

		void Add(const T& Item, int32 Weight = 1)
		{
			Items.Add(Item);
			Weights.Add(Weight);
		}

		void SetWeight(int32 Index, int32 Weight) { Weights[Index] = Weight; }

		/** The next item. The bag must have one with weight. */
		const T& Draw()
		{
			const int32 Index = Round.Draw(Weights, bNoRepeatAcrossRounds, &Stream);
			checkf(Index != INDEX_NONE, TEXT("TShuffleBag::Draw on a bag with nothing to draw"));
			return Items[Index];
		}

		/** The next draw deals a new round. */
		void Reset() { Round.Reset(); }

		void Empty()
		{
			Items.Empty();
			Weights.Empty();
			Round.Reset();
		}

		bool IsEmpty() const { return Items.IsEmpty(); }
		int32 Num() const { return Items.Num(); }

		/** A new round doesn't open with the item the last one closed with, for items that are at most half of a round. */
		bool bNoRepeatAcrossRounds = true;

	private:
		TArray<T> Items;
		TArray<int32> Weights;
		FKzShuffleRound Round;
		FRandomStream Stream;
	};
}
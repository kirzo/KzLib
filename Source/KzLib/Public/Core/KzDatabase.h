// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzParamDef.h"
#include "Core/KzPropertyBagHelpers.h"
#include "KzDatabase.generated.h"

/**
 * Defines criteria to search and filter database items.
 * Includes support for hard requirements, exclusions, and heuristic sorting (scoring).
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabaseQuery
{
	GENERATED_BODY()

	/** The item MUST have all these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer RequireTags;

	/** The item MUST NOT have any of these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer IgnoreTags;

	/*
	 * Heuristic Scoring: Items are sorted by how many of these tags they possess.
	 * Useful for finding the "Best Match".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagContainer OptionalTags;

	/** Advanced logic (A OR B AND C). Checked after Require/Ignore tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
	FGameplayTagQuery TagQuery;

	/** Checks if a container satisfies the hard requirements (Require, Ignore, TagQuery) */
	bool Matches(const FGameplayTagContainer& Tags) const;

	/** Returns a score based on the number of matching OptionalTags */
	int32 CalculateScore(const FGameplayTagContainer& Tags) const;

	/** Returns true if the query has no constraints */
	bool IsEmpty() const;
};

/** A single entry in the database. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabaseItem
{
	GENERATED_BODY()

	/** Unique Identifier for this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FName ID = NAME_None;

	/** Semantic tags for querying. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FGameplayTagContainer Tags;

	/** The dynamic storage. */
	UPROPERTY(EditAnywhere, Category = "Data", meta = (FixedLayout, ShowOnlyInnerProperties))
	FInstancedPropertyBag Data;

public:
	/**
	 * Ensures the internal storage matches the Definition.
	 * Must be called when creating the item or changing the Def.
	 */
	void SyncType(const FKzParamDef& Def);

	/** Returns true if the bag has a valid value set */
	bool IsValid() const
	{
		return Data.IsValid();
	}

	template <typename T>
	TValueOrError<T, EPropertyBagResult> GetValue() const
	{
		return KzPropertyBag::Get<T>(Data, FName("Data"));
	}

	template <typename T>
	void SetValue(const T& InValue)
	{
		KzPropertyBag::Set<T>(Data, FName("Data"), InValue);
	}
};

/**
 * A container acting as a database registry.
 * It defines the Type for all its items.
 */
USTRUCT(BlueprintType)
struct KZLIB_API FKzDatabase
{
	GENERATED_BODY()

	/** Defines the type of data stored in this database. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database", meta = (HideName, NoArrays))
	FKzParamDef Type;

	/** The collection of items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database", meta = (TitleProperty = "ID"))
	TArray<FKzDatabaseItem> Items;

	FKzDatabase()
	{
		Type.ValueType = EPropertyBagPropertyType::None;
	}

	/** Configures the Database Schema based on a C++ Type. */
	template <typename T>
	void InitType()
	{
		// Unwrap Array (e.g., TArray<int32> -> int32)
		using UnwrappedT = typename KzPropertyBag::Private::TUnwrapArray<T>::Type;
		constexpr bool bIsArray = KzPropertyBag::Private::TUnwrapArray<T>::IsArray;

		// Get Compile-Time Info from the Inner Type
		// We use std::decay_t to remove const/volatile/references just in case
		using Traits = KzPropertyBag::TPropertyBagType<std::decay_t<UnwrappedT>>;

		Type.ValueType = Traits::Type;
		Type.ValueTypeObject = Traits::GetObjectType();
		Type.ContainerType = bIsArray ? EPropertyBagContainerType::Array : EPropertyBagContainerType::None;

		Items.Empty();
	}

	/** Clears all items */
	void Empty()
	{
		Items.Empty();
	}

	/** Returns true if there are no items */
	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}

	/** Removes an item by ID. Returns true if removed. */
	bool RemoveItem(FName ID)
	{
		return Items.RemoveAll([&ID](const FKzDatabaseItem& Item) { return Item.ID == ID; }) > 0;
	}

	/**
	 * Adds or Updates an item with type safety checks.
	 * Returns nullptr if the provided type T does not match the Database Definition.
	 */
	template <typename T>
	FKzDatabaseItem* AddOrUpdateItem(FName ID, const FGameplayTagContainer& Tags, const T& Value)
	{
		// Get Compile-Time Type Info from T
		using Traits = KzPropertyBag::TPropertyBagType<T>;
		const EPropertyBagPropertyType InputType = Traits::Type;
		const UObject* InputObj = Traits::GetObjectType();

		// Validate against Runtime Schema (this->Type)
		bool bCompatible = (Type.ValueType == InputType);

		if (bCompatible && (InputType == EPropertyBagPropertyType::Object ||
			InputType == EPropertyBagPropertyType::SoftObject ||
			InputType == EPropertyBagPropertyType::Class ||
			InputType == EPropertyBagPropertyType::SoftClass))
		{
			// For objects, allow Child -> Parent assignment
			const UClass* SchemaClass = Cast<UClass>(Type.ValueTypeObject);
			const UClass* InputClass = Cast<UClass>(InputObj);
			if (SchemaClass && InputClass)
			{
				bCompatible = InputClass->IsChildOf(SchemaClass);
			}
			else
			{
				bCompatible = (Type.ValueTypeObject == InputObj);
			}
		}
		else if (bCompatible && (InputType == EPropertyBagPropertyType::Struct || InputType == EPropertyBagPropertyType::Enum))
		{
			// Structs and Enums must match exactly
			bCompatible = (Type.ValueTypeObject == InputObj);
		}

		if (!ensureMsgf(bCompatible, TEXT("FKzDatabase Type Mismatch: Trying to add type '%d' object '%s' to DB expecting '%d' object '%s'"),
			(int32)InputType, InputObj ? *InputObj->GetName() : TEXT("Null"),
			(int32)Type.ValueType, Type.ValueTypeObject ? *Type.ValueTypeObject->GetName() : TEXT("Null")))
		{
			return nullptr;
		}

		// Perform Operation
		FKzDatabaseItem* Item = FindItem(ID);

		if (!Item)
		{
			Item = &Items.AddDefaulted_GetRef();
			Item->ID = ID;
		}

		Item->Tags = Tags;
		Item->SyncType(Type);
		Item->SetValue<T>(Value);

		return Item;
	}

	/** Finds an item by ID */
	const FKzDatabaseItem* FindItem(FName ID) const;
	FKzDatabaseItem* FindItem(FName ID);

	/**
	 * Finds all items matching the query.
	 * Results are sorted by Score (OptionalTags match count) descending.
	 * @return Number of items found.
	 */
	int32 QueryItems(const FKzDatabaseQuery& Query, TArray<const FKzDatabaseItem*>& OutItems) const;
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzSerializationLibrary.generated.h"

/** Modes available for serializing objects. */
UENUM(BlueprintType)
enum class EKzSerializationMode : uint8
{
	// Native Unreal saving. Serializes only properties explicitly marked with the "SaveGame" flag.
	SaveGameProperties,

	// Delta saving. Compares the object against its Class Default Object (CDO) and saves only the modified variables.
	ModifiedProperties
};

/** Data container used to store binary serialized objects. */
USTRUCT(BlueprintType)
struct KZLIB_API FKzSerializedData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KzLib|Serialization")
	TArray<uint8> Data;

	FORCEINLINE void Clear()
	{
		Data.Empty();
	}
};

/** Blueprint library to handle object serialization to binary data. */
UCLASS()
class KZLIB_API UKzSerializationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Serializes properties from a given object into a binary data container.
	 * @param Object The object to serialize.
	 * @param Mode The serialization strategy to use.
	 * @return A struct containing the binary array.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Serialization")
	static FKzSerializedData SerializeObject(UObject* Object, EKzSerializationMode Mode);

	/**
	 * Deserializes properties from a binary data container back into an object.
	 * @param Object The object to populate with the saved data.
	 * @param SerializedData The binary data to read from.
	 * @param Mode The serialization strategy used when saving.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Serialization")
	static void DeserializeObject(UObject* Object, UPARAM(ref) FKzSerializedData& SerializedData, EKzSerializationMode Mode);

	/**
	 * Resets all UPROPERTYs with the CPF_SaveGame flag to their class default value.
	 * @param Object The object to reset.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Serialization")
	static void ResetSaveGameProperties(UObject* Object);

	/**
	 * Spawns an actor of the specified class in a deferred manner, applies the serialized data,
	 * and then finishes the spawning process so BeginPlay runs with the restored state.
	 * @param WorldContextObject Context required to access the world.
	 * @param ActorClass The class of the actor to spawn.
	 * @param SpawnTransform The location and rotation to spawn the actor at.
	 * @param SerializedData The binary data to inject before BeginPlay.
	 * @return The newly spawned and restored Actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Serialization", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ActorClass"))
	static AActor* SpawnActorWithData(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, UPARAM(ref) FKzSerializedData& SerializedData);

	/**
	 * Spawns a new actor by serializing an existing actor instance and applying its state to the new one.
	 * Acts as a deep clone using the serialization system.
	 * @param WorldContextObject Context required to access the world.
	 * @param TemplateActor The actor instance to copy the state from.
	 * @param SpawnTransform The location and rotation to spawn the new actor at.
	 * @param Mode The serialization strategy to use when copying the state.
	 * @return The newly spawned and restored Actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Serialization", meta = (WorldContext = "WorldContextObject"))
	static AActor* SpawnActorFromInstance(const UObject* WorldContextObject, AActor* TemplateActor, const FTransform& SpawnTransform, EKzSerializationMode Mode);
};
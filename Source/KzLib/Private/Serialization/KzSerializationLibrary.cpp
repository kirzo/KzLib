// Copyright 2026 kirzo

#include "Serialization/KzSerializationLibrary.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

struct FKzSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	FKzSaveGameArchive(FArchive& InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, false)
	{
		ArIsSaveGame = true;
		ArNoDelta = true;
	}
};

FKzSerializedData UKzSerializationLibrary::SerializeObject(UObject* Object, EKzSerializationMode Mode)
{
	FKzSerializedData SerializedData;

	if (!Object)
	{
		return SerializedData;
	}

	switch (Mode)
	{
		case EKzSerializationMode::SaveGameProperties:
		{
			FMemoryWriter MemoryWriter(SerializedData.Data, true);
			FKzSaveGameArchive Ar(MemoryWriter);
			Object->Serialize(Ar);
			break;
		}
		case EKzSerializationMode::ModifiedProperties:
		{
			FMemoryWriter MemoryWriter(SerializedData.Data, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, true);

			// Get a pointer to the object's archetype (class default object)
			UObject* Archetype = Object->GetArchetype();

			// Iterate over the properties of the object
			for (FProperty* Property = Object->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext)
			{
				// Only process editable properties
				if (Property->HasAnyPropertyFlags(CPF_Edit) && !Property->HasAnyPropertyFlags(CPF_EditConst))
				{
					void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
					void* ArchetypeValuePtr = Property->ContainerPtrToValuePtr<void>(Archetype);

					// Check if the property has a different value in the object compared to its archetype
					if (!Property->Identical(ValuePtr, ArchetypeValuePtr))
					{
						// 1. Serialize the property name
						FString PropertyName = Property->GetName();
						Ar << PropertyName;

						// 2. Serialize the property data into a temporary buffer. 
						// We use a temporary buffer to safely capture dynamic sizes (like TArrays or FStrings)
						// and to measure the exact byte payload size for forward compatibility.
						TArray<uint8> TempPayload;
						FMemoryWriter TempWriter(TempPayload, true);
						FObjectAndNameAsStringProxyArchive TempAr(TempWriter, true);

						Property->SerializeItem(FStructuredArchiveFromArchive(TempAr).GetSlot(), ValuePtr);

						// 3. Write the payload size, then the payload itself
						int32 PayloadSize = TempPayload.Num();
						Ar << PayloadSize;
						Ar.Serialize(TempPayload.GetData(), PayloadSize);
					}
				}
			}
			break;
		}
	}

	return SerializedData;
}

void UKzSerializationLibrary::DeserializeObject(UObject* Object, UPARAM(ref) FKzSerializedData& SerializedData, EKzSerializationMode Mode)
{
	if (!Object || SerializedData.Data.Num() == 0)
	{
		return;
	}

	switch (Mode)
	{
		case EKzSerializationMode::SaveGameProperties:
		{
			FMemoryReader MemoryReader(SerializedData.Data, true);
			FKzSaveGameArchive Ar(MemoryReader);
			Object->Serialize(Ar);
			break;
		}
		case EKzSerializationMode::ModifiedProperties:
		{
			FMemoryReader MemoryReader(SerializedData.Data, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);

			// Read until the archive is empty or an error occurs
			while (!Ar.AtEnd() && !Ar.IsError())
			{
				// 1. Read the property name and the payload size
				FString PropertyName;
				Ar << PropertyName;

				int32 PayloadSize;
				Ar << PayloadSize;

				// Check if the property still exists in the code (it might have been removed in a patch)
				FProperty* ObjectProperty = FindFProperty<FProperty>(Object->GetClass(), *PropertyName);
				if (ObjectProperty)
				{
					// Extract the payload buffer
					TArray<uint8> TempPayload;
					TempPayload.SetNumUninitialized(PayloadSize);
					Ar.Serialize(TempPayload.GetData(), PayloadSize);

					// Deserialize the buffer into the actual property safely
					FMemoryReader TempReader(TempPayload, true);
					FObjectAndNameAsStringProxyArchive TempAr(TempReader, true);

					void* ValuePtr = ObjectProperty->ContainerPtrToValuePtr<void>(Object);
					ObjectProperty->SerializeItem(FStructuredArchiveFromArchive(TempAr).GetSlot(), ValuePtr);
				}
				else
				{
					// Property was removed or renamed in a newer version.
					// Safely skip its payload bytes to avoid corrupting the rest of the archive.
					Ar.Seek(Ar.Tell() + PayloadSize);
				}
			}
			break;
		}
	}
}

void UKzSerializationLibrary::ResetSaveGameProperties(UObject* Object)
{
	if (!Object)
	{
		return;
	}

	UClass* Class = Object->GetClass();
	UObject* ClassDefaults = Class->GetDefaultObject();

	for (FProperty* Property = Class->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		// Restore only properties explicitly marked to be saved
		const bool bIsSaveGame = Property->HasAnyPropertyFlags(CPF_SaveGame);
		if (bIsSaveGame)
		{
			Property->CopyCompleteValue_InContainer(Object, ClassDefaults);
		}
	}
}

AActor* UKzSerializationLibrary::SpawnActorWithData(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, UPARAM(ref) FKzSerializedData& SerializedData)
{
	if (!WorldContextObject || !ActorClass)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 1. Begin deferred spawning. The actor is created but BeginPlay is NOT called yet.
	AActor* DeferredActor = World->SpawnActorDeferred<AActor>(ActorClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (DeferredActor)
	{
		// 2. Inject the binary data into the actor before it initializes.
		DeserializeObject(DeferredActor, SerializedData, EKzSerializationMode::SaveGameProperties);

		// 3. Finish spawning. This will trigger OnConstruction and BeginPlay with the restored variables.
		UGameplayStatics::FinishSpawningActor(DeferredActor, SpawnTransform);
	}

	return DeferredActor;
}

AActor* UKzSerializationLibrary::SpawnActorFromInstance(const UObject* WorldContextObject, AActor* TemplateActor, const FTransform& SpawnTransform, EKzSerializationMode Mode)
{
	if (!TemplateActor)
	{
		return nullptr;
	}

	// 1. Serialize the state of the provided template actor into memory
	FKzSerializedData CopiedData = SerializeObject(TemplateActor, Mode);

	// 2. Delegate the spawning and deserialization to the base function using the template's class
	return SpawnActorWithData(WorldContextObject, TemplateActor->GetClass(), SpawnTransform, CopiedData);
}
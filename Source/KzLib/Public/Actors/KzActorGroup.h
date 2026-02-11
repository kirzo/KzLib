// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KzActorGroup.generated.h"

class UBillboardComponent;

/** Component used exclusively to hook the Editor Visualizer for KzActorGroup. */
UCLASS()
class KZLIB_API UKzActorGroupComponent : public UActorComponent
{
	GENERATED_BODY()
};

/** A simple grouping actor that holds references to other actors. */
UCLASS(HideDropdown, NotBlueprintable, HideCategories(Rendering, Replication, Collision, HLOD, Physics, Networking, Input, Actor, LevelInstance, Cooking))
class KZLIB_API AKzActorGroup : public AActor
{
	GENERATED_BODY()

public:
	/** List of actors belonging to this group. Editable via the picker in the level editor. */
	UPROPERTY(Category = "Group", EditInstanceOnly, BlueprintReadOnly)
	TArray<AActor*> Actors;

protected:
#if WITH_EDITORONLY_DATA
	/** Component required to draw debug information using KzActorGroupVisualizer */
	UPROPERTY()
	TObjectPtr<UKzActorGroupComponent> GroupComponent;

	/** Billboard used to see and select the group actor in the editor */
	UPROPERTY(Category = "Group", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif

public:
	AKzActorGroup();
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
};
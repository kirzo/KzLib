// Copyright 2026 kirzo

#include "Actors/KzActorGroup.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AKzActorGroup::AKzActorGroup()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	// Initialize the visualization hook component
	GroupComponent = CreateEditorOnlyDefaultSubobject<UKzActorGroupComponent>(TEXT("GroupComponent"));

	// Initialize the sprite (Billboard)
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> GroupTextureObject;
			FName ID_Group;
			FText NAME_Group;
			FConstructorStatics()
				: GroupTextureObject(TEXT("/KzLib/EditorResources/ActorGroup"))
				, ID_Group(TEXT("Group"))
				, NAME_Group(NSLOCTEXT("SpriteCategory", "Groups", "Groups"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.GroupTextureObject.Get();
		SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->SetupAttachment(RootComponent);
	}
#endif // WITH_EDITORONLY_DATA
}

void AKzActorGroup::BeginPlay()
{
	Super::BeginPlay();

	// Clean up invalid references on play
	Actors.Remove(nullptr);
}

void AKzActorGroup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Lock scale to 1.0 as scaling the group shouldn't affect the logic usually
	SetActorScale3D(FVector(1.0f));
}
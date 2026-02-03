// Copyright 2026 kirzo

#include "Customizations/KzComponentSocketReferenceCustomization.h"
#include "Components/KzComponentSocketReference.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/SListView.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/Actor.h"
#include "Engine/BlueprintGeneratedClass.h"

#define LOCTEXT_NAMESPACE "KzComponentSocketReferenceCustomization"

TSharedRef<IPropertyTypeCustomization> FKzComponentSocketReferenceCustomization::MakeInstance()
{
	return MakeShareable(new FKzComponentSocketReferenceCustomization());
}

AActor* FKzComponentSocketReferenceCustomization::ResolveActorContext(UObject* Obj) const
{
	UObject* Current = Obj;

	// Traverse up the outer chain
	while (Current)
	{
		// Is it an Actor? (Instance or CDO)
		if (AActor* Actor = Cast<AActor>(Current))
		{
			return Actor;
		}

		// Is it a Component?
		if (UActorComponent* Comp = Cast<UActorComponent>(Current))
		{
			// If it's a component instance in the world, it has an Owner.
			if (AActor* Owner = Comp->GetOwner())
			{
				return Owner;
			}
			// If it's a component template inside a BP, we need to keep going up 
			// (Comp -> SCS -> BPGC) or check its direct outer.
		}

		// Is it a Blueprint Generated Class? 
		// (This happens when we traverse up from a Component Template or Instanced Object in the BP Editor)
		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(Current))
		{
			// Return the CDO so we can inspect native components later, 
			// and use the class for SCS lookups.
			return Cast<AActor>(BPClass->GetDefaultObject());
		}

		// Move up
		Current = Current->GetOuter();
	}

	return nullptr;
}

AActor* FKzComponentSocketReferenceCustomization::GetTargetActor() const
{
	// Check Override Actor (Highest Priority - Instance Only)
	UObject* OverrideObj = nullptr;
	if (OverrideActorHandle.IsValid() && OverrideActorHandle->GetValue(OverrideObj) == FPropertyAccess::Success)
	{
		if (AActor* Override = Cast<AActor>(OverrideObj))
		{
			return Override;
		}
	}

	// Resolve Context from the Property Outer
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() > 0)
	{
		return ResolveActorContext(OuterObjects[0]);
	}

	return nullptr;
}

void FKzComponentSocketReferenceCustomization::GetAllowedComponentClasses(TArray<UClass*>& OutClasses) const
{
	OutClasses.Reset();

	if (!StructPropertyHandle.IsValid() || !StructPropertyHandle->GetProperty())
	{
		return;
	}

	const FString& AllowedClassesMeta = StructPropertyHandle->GetProperty()->GetMetaData(TEXT("AllowedClasses"));
	if (AllowedClassesMeta.IsEmpty())
	{
		return;
	}

	TArray<FString> ClassNames;
	AllowedClassesMeta.ParseIntoArray(ClassNames, TEXT(","), true);

	for (FString& Name : ClassNames)
	{
		Name.TrimStartAndEndInline();
		if (Name.IsEmpty()) continue;

		UClass* FoundClass = nullptr;

		// 1. Try to find the class using the full path or loose name
		FoundClass = UClass::TryFindTypeSlow<UClass>(Name);

		// 2. If not found and it looks like a path, try to load it
		if (!FoundClass && (Name.StartsWith(TEXT("/")) || Name.Contains(TEXT("."))))
		{
			FoundClass = LoadObject<UClass>(nullptr, *Name);
		}

		if (FoundClass)
		{
			OutClasses.Add(FoundClass);
		}
	}
}

void FKzComponentSocketReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;
	OverrideActorHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, OverrideActor));
	ComponentNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, ComponentName));
	SocketNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, SocketName));
	RelativeLocationHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, RelativeLocation));
	RelativeRotationHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, RelativeRotation));

	if (OverrideActorHandle.IsValid())
	{
		OverrideActorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnOverrideActorChanged));
	}

	// Determine if we have a valid context to show pickers
	bool bHasValidContext = false;
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() > 0)
	{
		// Try to resolve an actor context from the immediate outer
		if (ResolveActorContext(OuterObjects[0]) != nullptr)
		{
			bHasValidContext = true;
		}
	}

	TSharedPtr<SHorizontalBox> HBox = SNew(SHorizontalBox);

	if (bHasValidContext)
	{
		// Smart Pickers
		HBox->AddSlot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SComboButton)
					.OnGetMenuContent(this, &FKzComponentSocketReferenceCustomization::OnGetComponentsMenu)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(this, &FKzComponentSocketReferenceCustomization::GetCurrentComponentName)
							.Font(IDetailLayoutBuilder::GetDetailFont())
					]
			];

		HBox->AddSlot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SComboButton)
					.Visibility(this, &FKzComponentSocketReferenceCustomization::GetSocketVisibility)
					.OnGetMenuContent(this, &FKzComponentSocketReferenceCustomization::OnGetSocketsMenu)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(this, &FKzComponentSocketReferenceCustomization::GetCurrentSocketName)
							.Font(IDetailLayoutBuilder::GetDetailFont())
					]
			];
	}
	else
	{
		// Fallback Text Fields
		HBox->AddSlot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ComponentNameHandle->CreatePropertyValueWidget()
			];

		HBox->AddSlot()
			.FillWidth(1.0f)
			[
				SocketNameHandle->CreatePropertyValueWidget()
			];
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(300.0f)
		.MaxDesiredWidth(600.0f)
		[
			HBox.ToSharedRef()
		];
}

void FKzComponentSocketReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Check Metadata "NoOffset"
	bool bHideOffset = false;
	if (const FProperty* Prop = PropertyHandle->GetProperty())
	{
		bHideOffset = Prop->HasMetaData(TEXT("NoOffset"));
	}

	// Resolve Context for "OverrideActor" visibility
	bool bIsInstance = false;
	if (AActor* ContextActor = GetTargetActor())
	{
		// If the resolved actor is NOT a CDO/Archetype, we are in an instance context
		if (!ContextActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			bIsInstance = true;
		}
	}

	// Override Actor
	if (bIsInstance && OverrideActorHandle.IsValid())
	{
		ChildBuilder.AddProperty(OverrideActorHandle.ToSharedRef());
	}

	// Relative Location & Rotation (Only if NoOffset is NOT present)
	if (!bHideOffset)
	{
		if (RelativeLocationHandle.IsValid())
		{
			ChildBuilder.AddProperty(RelativeLocationHandle.ToSharedRef());
		}

		if (RelativeRotationHandle.IsValid())
		{
			ChildBuilder.AddProperty(RelativeRotationHandle.ToSharedRef());
		}
	}
}

void FKzComponentSocketReferenceCustomization::OnOverrideActorChanged()
{
	ComponentNameHandle->SetValue(FName(NAME_None));
	SocketNameHandle->SetValue(FName(NAME_None));
}

void FKzComponentSocketReferenceCustomization::BuildComponentList(TArray<FName>& OutNames)
{
	AActor* Target = GetTargetActor();
	if (!Target) return;

	// Parse Allowed Classes from Metadata
	TArray<UClass*> AllowedClasses;
	GetAllowedComponentClasses(AllowedClasses);

	// Helper lambda to check if a component class matches the filter
	auto IsClassAllowed = [&](const UClass* InClass) -> bool
		{
			// Always require USceneComponent (base requirement of the struct)
			if (!InClass || !InClass->IsChildOf(USceneComponent::StaticClass()))
			{
				return false;
			}

			// If no filter specified, allow all SceneComponents
			if (AllowedClasses.IsEmpty())
			{
				return true;
			}

			// Check against allowed list
			for (UClass* Allowed : AllowedClasses)
			{
				if (InClass->IsChildOf(Allowed))
				{
					return true;
				}
			}

			return false;
		};

	// Native / Instance Components
	for (UActorComponent* Comp : Target->GetComponents())
	{
		if (IsClassAllowed(Comp->GetClass()))
		{
			OutNames.Add(Comp->GetFName());
		}
	}

	// Blueprint Added Components (SCS) - Only needed if Target is a CDO
	if (Target->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(Target->GetClass()))
		{
			if (USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
			{
				for (USCS_Node* Node : SCS->GetAllNodes())
				{
					if (Node && IsClassAllowed(Node->ComponentClass))
					{
						OutNames.Add(Node->GetVariableName());
					}
				}
			}
		}
	}
}

TSharedRef<SWidget> FKzComponentSocketReferenceCustomization::OnGetComponentsMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<FName> Names;
	BuildComponentList(Names);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("None", "None"),
		LOCTEXT("NoneTooltip", "Clear component reference"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnComponentSelected, FName(NAME_None)))
	);

	MenuBuilder.AddSeparator();

	for (const FName& Name : Names)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromName(Name),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnComponentSelected, Name))
		);
	}

	return MenuBuilder.MakeWidget();
}

void FKzComponentSocketReferenceCustomization::OnComponentSelected(FName InName)
{
	FName CurrentName;
	ComponentNameHandle->GetValue(CurrentName);

	if (CurrentName == InName) return;

	ComponentNameHandle->SetValue(InName);
	SocketNameHandle->SetValue(FName(NAME_None));
}

FText FKzComponentSocketReferenceCustomization::GetCurrentComponentName() const
{
	FName CurrentName;
	ComponentNameHandle->GetValue(CurrentName);

	// Case A: The user has explicitly selected a component.
	if (!CurrentName.IsNone())
	{
		return FText::FromName(CurrentName);
	}

	// Case B: No selection (Default behavior).
	// We calculate what "None" resolves to, to give the user immediate feedback.

	FString ResolvedContextName = TEXT("None");

	// 1. Check if there is an Override Actor (It takes priority).
	UObject* OverrideObj = nullptr;
	if (OverrideActorHandle.IsValid() && OverrideActorHandle->GetValue(OverrideObj) == FPropertyAccess::Success && OverrideObj)
	{
		if (AActor* OverrideActor = Cast<AActor>(OverrideObj))
		{
			// If referencing an external actor, default is usually its Root.
			ResolvedContextName = OverrideActor->GetRootComponent() ? OverrideActor->GetRootComponent()->GetName() : TEXT("Actor Root");
		}
	}
	else
	{
		// 2. Resolve Context from the Property Outer (Smart Recursion).
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);

		if (OuterObjects.Num() > 0)
		{
			const UObject* Current = OuterObjects[0];

			// Traverse up the hierarchy to find the nearest SceneComponent or Actor.
			while (Current)
			{
				// If the struct is inside a SceneComponent (or is one), that IS the default context.
				if (const USceneComponent* Comp = Cast<USceneComponent>(Current))
				{
					ResolvedContextName = Comp->GetName();

					// Clean up variable names in SCS (Blueprint Editor) which often have "_GEN_VARIABLE" suffixes.
					// This is purely cosmetic.
					if (ResolvedContextName.EndsWith(TEXT("_GEN_VARIABLE")))
					{
						ResolvedContextName.LeftChopInline(13); // Remove suffix
					}
					break;
				}

				// If we hit the Actor, the default is the RootComponent.
				if (const AActor* Act = Cast<AActor>(Current))
				{
					if (USceneComponent* Root = Act->GetRootComponent())
					{
						ResolvedContextName = Root->GetName();
					}
					else
					{
						ResolvedContextName = TEXT("Actor Root");
					}
					break;
				}

				Current = Current->GetOuter();
			}
		}
	}

	// Return formatted string: "Default (MeshName)"
	return FText::Format(LOCTEXT("DefaultContextFmt", "Default ({0})"), FText::FromString(ResolvedContextName));
}

USceneComponent* FKzComponentSocketReferenceCustomization::FindComponentByName(FName Name) const
{
	if (Name.IsNone()) return nullptr;

	AActor* Target = GetTargetActor();
	if (!Target) return nullptr;

	// Search Native/Instance Components
	for (UActorComponent* Comp : Target->GetComponents())
	{
		if (Comp->GetFName() == Name) return Cast<USceneComponent>(Comp);
	}

	// Search SCS (BP CDO Fallback)
	if (Target->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(Target->GetClass()))
		{
			if (USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
			{
				for (USCS_Node* Node : SCS->GetAllNodes())
				{
					if (Node->GetVariableName() == Name)
					{
						return Cast<USceneComponent>(Node->GetActualComponentTemplate(BPClass));
					}
				}
			}
		}
	}

	return nullptr;
}

TSharedRef<SWidget> FKzComponentSocketReferenceCustomization::OnGetSocketsMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	FName CurrentCompName;
	ComponentNameHandle->GetValue(CurrentCompName);
	USceneComponent* Comp = FindComponentByName(CurrentCompName);

	if (Comp)
	{
		TArray<FName> Sockets = Comp->GetAllSocketNames();

		MenuBuilder.AddMenuEntry(
			LOCTEXT("NoSocket", "None (Component Origin)"),
			LOCTEXT("NoSocketTooltip", "Use the component's pivot"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnSocketSelected, FName(NAME_None)))
		);

		MenuBuilder.AddSeparator();

		for (const FName& Socket : Sockets)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromName(Socket),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnSocketSelected, Socket))
			);
		}
	}

	return MenuBuilder.MakeWidget();
}

void FKzComponentSocketReferenceCustomization::OnSocketSelected(FName InName)
{
	SocketNameHandle->SetValue(InName);
}

FText FKzComponentSocketReferenceCustomization::GetCurrentSocketName() const
{
	FName Val;
	SocketNameHandle->GetValue(Val);
	return Val.IsNone() ? LOCTEXT("SelectSocket", "Select Socket...") : FText::FromName(Val);
}

EVisibility FKzComponentSocketReferenceCustomization::GetSocketVisibility() const
{
	FName Val;
	ComponentNameHandle->GetValue(Val);
	return Val.IsNone() ? EVisibility::Collapsed : EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
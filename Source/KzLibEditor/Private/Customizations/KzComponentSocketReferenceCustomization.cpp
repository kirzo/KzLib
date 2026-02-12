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

	bool bHideSocket = false;
	if (const FProperty* Prop = PropertyHandle->GetProperty())
	{
		bHideSocket = Prop->HasMetaData(TEXT("NoSocket"));
	}

	// If NoSocket is requested, ensure the underlying data is cleared to avoid confusion
	// (Optional safety step, though mostly visual hiding is enough)
	if (bHideSocket)
	{
		FName CurrentSocket;
		SocketNameHandle->GetValue(CurrentSocket);
		if (!CurrentSocket.IsNone())
		{
			SocketNameHandle->SetValue(FName(NAME_None));
		}
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
		HBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.WarningWithColor"))
					.Visibility(this, &FKzComponentSocketReferenceCustomization::GetComponentWarningVisibility)
					.ToolTipText(LOCTEXT("CompNotFoundTooltip", "Component not found! It may have been renamed or deleted."))
			];

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

		if (!bHideSocket)
		{
			HBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.WarningWithColor"))
						.Visibility(this, &FKzComponentSocketReferenceCustomization::GetSocketWarningVisibility)
						.ToolTipText(LOCTEXT("SocketNotFoundTooltip", "Socket not found on the selected component!"))
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

		if (!bHideSocket)
		{
			HBox->AddSlot()
				.FillWidth(1.0f)
				[
					SocketNameHandle->CreatePropertyValueWidget()
				];
		}
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

	// --- Helper to resolve Variable Name from Component Instance ---
	auto GetComponentVariableName = [](AActor* InOwner, UActorComponent* InComp) -> FString
		{
			if (!InOwner || !InComp) return "";

			// 1. Iterate Object Properties in the Actor Class to find which one points to InComp
			for (TFieldIterator<FObjectProperty> It(InOwner->GetClass()); It; ++It)
			{
				// Skip properties that are not components or match our type
				if (!It->PropertyClass->IsChildOf(UActorComponent::StaticClass())) continue;

				// Check if the pointer value matches our component instance
				if (It->GetObjectPropertyValue_InContainer(InOwner) == InComp)
				{
					return It->GetName();
				}
			}

			// 2. Fallback: If no property points to it (e.g. dynamically created), use the instance name.
			return InComp->GetName();
		};

	// --- Recursive Collector ---
	// ContextActor: The actor we are currently inspecting (Root or Child)
	// NamePrefix: The dot-notation path accumulated so far (e.g. "MyChild.")
	// bIsCDO: Optimization to handle template lookup differently
	TFunction<void(AActor*, FString)> CollectFromActor =
		[&](AActor* ContextActor, FString NamePrefix)
		{
			if (!ContextActor) return;

			// A. Collect Native/Instance Components
			for (UActorComponent* Comp : ContextActor->GetComponents())
			{
				USceneComponent* SceneComp = Cast<USceneComponent>(Comp);
				if (!SceneComp) continue;

				// Reverse lookup to get the Variable Name (e.g. "Mesh" instead of "CharacterMesh0")
				FString CompName = GetComponentVariableName(ContextActor, Comp);
				if (CompName.EndsWith(TEXT("_GEN_VARIABLE"))) CompName.LeftChopInline(13); // Clean BP garbage

				FString FullPath = NamePrefix + CompName;

				// Handle ChildActorComponent Special Case
				if (UChildActorComponent* ChildActorComp = Cast<UChildActorComponent>(Comp))
				{
					// Only list the ChildActorComponent itself if it passes the filter AND we want to allow attaching to the container.
					if (IsClassAllowed(ChildActorComp->GetClass()))
					{
						OutNames.Add(FName(*FullPath));
					}

					// Recurse into Child
					AActor* ChildActor = ChildActorComp->GetChildActor();

					// CDO Handling: If the child actor doesn't exist yet (template), check the Class CDO
					if (!ChildActor && ContextActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
					{
						if (ChildActorComp->GetChildActorClass())
						{
							ChildActor = ChildActorComp->GetChildActorClass()->GetDefaultObject<AActor>();
						}
					}

					if (ChildActor)
					{
						// Recurse: Add dot to prefix
						CollectFromActor(ChildActor, FullPath + TEXT("."));
					}
				}
				else
				{
					// Standard Component
					if (IsClassAllowed(Comp->GetClass()))
					{
						OutNames.Add(FName(*FullPath));
					}
				}
			}

			// B. Collect SCS Nodes (Only for BP CDOs)
			if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(ContextActor->GetClass()))
			{
				if (USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
				{
					for (USCS_Node* Node : SCS->GetAllNodes())
					{
						if (!Node) continue;

						// Check if we already processed this via Native loop (ComponentTemplate is often in GetComponents)
						// To avoid duplicates, we can check names.
						FString NodeName = Node->GetVariableName().ToString();
						FString FullPath = NamePrefix + NodeName;

						// Simple deduplication check: usually not needed if we trust the lists don't overlap much in CDO context, 
						// but let's be safe:
						if (OutNames.Contains(FName(*FullPath))) continue;

						UActorComponent* Template = Node->GetActualComponentTemplate(BPClass);
						if (!Template) continue;

						if (UChildActorComponent* ChildActorTemplate = Cast<UChildActorComponent>(Template))
						{
							// ... Same ChildActor Logic for SCS ...
							if (IsClassAllowed(ChildActorTemplate->GetClass()))
							{
								OutNames.Add(FName(*FullPath));
							}

							if (ChildActorTemplate->GetChildActorClass())
							{
								AActor* ChildCDO = ChildActorTemplate->GetChildActorClass()->GetDefaultObject<AActor>();
								CollectFromActor(ChildCDO, FullPath + TEXT("."));
							}
						}
						else if (IsClassAllowed(Node->ComponentClass))
						{
							OutNames.Add(FName(*FullPath));
						}
					}
				}
			}
		};

	// Start Recursion
	CollectFromActor(Target, TEXT(""));
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

	// Case B: No selection. Determine if the implicit "Default" is valid.

	// 1. Get Allowed Classes
	TArray<UClass*> AllowedClasses;
	GetAllowedComponentClasses(AllowedClasses);

	auto IsClassAllowed = [&](const UClass* InClass) -> bool
		{
			if (!InClass) return false;
			// Always require USceneComponent base
			if (!InClass->IsChildOf(USceneComponent::StaticClass())) return false;

			// If no filter, everything is allowed
			if (AllowedClasses.IsEmpty()) return true;

			for (UClass* Allowed : AllowedClasses)
			{
				if (InClass->IsChildOf(Allowed)) return true;
			}
			return false;
		};

	// 2. Resolve the "Default" candidate component
	const USceneComponent* DefaultCandidate = nullptr;

	// Check Override Actor
	UObject* OverrideObj = nullptr;
	if (OverrideActorHandle.IsValid() && OverrideActorHandle->GetValue(OverrideObj) == FPropertyAccess::Success && OverrideObj)
	{
		if (AActor* OverrideActor = Cast<AActor>(OverrideObj))
		{
			DefaultCandidate = OverrideActor->GetRootComponent();
		}
	}
	else
	{
		// Resolve Context from Property Outer
		TArray<UObject*> OuterObjects;
		StructPropertyHandle->GetOuterObjects(OuterObjects);

		if (OuterObjects.Num() > 0)
		{
			const UObject* Current = OuterObjects[0];

			// Traverse up
			while (Current)
			{
				if (const USceneComponent* Comp = Cast<USceneComponent>(Current))
				{
					DefaultCandidate = Comp;
					break;
				}
				if (const AActor* Act = Cast<AActor>(Current))
				{
					DefaultCandidate = Act->GetRootComponent();
					break;
				}
				Current = Current->GetOuter();
			}
		}
	}

	// 3. Validate Candidate against Allowed Classes
	if (DefaultCandidate && IsClassAllowed(DefaultCandidate->GetClass()))
	{
		FString DisplayName = DefaultCandidate->GetName();

		// Clean up BP variable suffix for cleaner UI
		if (DisplayName.EndsWith(TEXT("_GEN_VARIABLE")))
		{
			DisplayName.LeftChopInline(13);
		}

		return FText::Format(LOCTEXT("DefaultContextFmt", "Default ({0})"), FText::FromString(DisplayName));
	}

	// 4. Fallback if default is invalid/restricted
	return LOCTEXT("SelectComponent", "Select Component...");
}

EVisibility FKzComponentSocketReferenceCustomization::GetComponentWarningVisibility() const
{
	FName CurrentName;
	if (ComponentNameHandle->GetValue(CurrentName) != FPropertyAccess::Success)
	{
		return EVisibility::Collapsed;
	}

	// 1. If name is None, it means "Use Default/Root", which is always valid contextually.
	if (CurrentName.IsNone())
	{
		return EVisibility::Collapsed;
	}

	// 2. Try to find the component
	USceneComponent* FoundComp = FindComponentByName(CurrentName);

	// 3. If not found, show warning
	return FoundComp ? EVisibility::Collapsed : EVisibility::Visible;
}

USceneComponent* FKzComponentSocketReferenceCustomization::FindComponentByName(FName Name) const
{
	if (Name.IsNone()) return nullptr;

	AActor* CurrentActor = GetTargetActor();
	if (!CurrentActor) return nullptr;

	FString CurrentPath = Name.ToString();
	FString Left, Right;

	// Loop while there are dots in the path (e.g., "WeaponActor.Muzzle" -> Split into "WeaponActor" and "Muzzle")
	while (CurrentPath.Split(TEXT("."), &Left, &Right))
	{
		// 1. Find the ChildActorComponent matching the 'Left' segment
		UChildActorComponent* TargetCAC = nullptr;
		FName TargetFName(*Left);

		// A. Try Property Lookup (Variable Name Match)
		if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(CurrentActor->GetClass(), TargetFName))
		{
			TargetCAC = Cast<UChildActorComponent>(Prop->GetObjectPropertyValue_InContainer(CurrentActor));
		}

		// B. Fallback: Search in Instance/Native components
		if (!TargetCAC)
		{
			for (UActorComponent* Comp : CurrentActor->GetComponents())
			{
				// Clean up BP variable suffixes if present in the component name
				FString CleanName = Comp->GetName();
				if (CleanName.EndsWith(TEXT("_GEN_VARIABLE"))) CleanName.LeftChopInline(13);

				if (CleanName == Left)
				{
					TargetCAC = Cast<UChildActorComponent>(Comp);
					break;
				}
			}
		}

		// C. Fallback: Search in SCS (Simple Construction Script) if we are in a BP CDO
		// This is needed because sometimes components in CDOs are not fully registered in GetComponents() yet
		if (!TargetCAC && CurrentActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(CurrentActor->GetClass()))
			{
				if (USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
				{
					if (USCS_Node* Node = SCS->FindSCSNode(FName(*Left)))
					{
						TargetCAC = Cast<UChildActorComponent>(Node->GetActualComponentTemplate(BPClass));
					}
				}
			}
		}

		// If we couldn't find the container component, the path is broken.
		if (!TargetCAC)
		{
			return nullptr;
		}

		// 2. Resolve the Actor inside the CAC
		AActor* NextActor = TargetCAC->GetChildActor();

		// If the child actor is not spawned (CDO), try the class default.
		if (!NextActor && TargetCAC->GetChildActorClass())
		{
			NextActor = TargetCAC->GetChildActorClass()->GetDefaultObject<AActor>();
		}

		if (!NextActor)
		{
			return nullptr; // Container exists, but the inner actor is missing/null.
		}

		// Advance to the next level
		CurrentActor = NextActor;
		CurrentPath = Right; // Continue processing the rest of the path
	}

	// --- Final Step: Find the Leaf Component in the final Actor ---
	// 'CurrentPath' now contains just the component name (e.g., "Muzzle")
	FName LeafFName(*CurrentPath);

	// A. Try Property Lookup (Variable Name)
	if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(CurrentActor->GetClass(), LeafFName))
	{
		return Cast<USceneComponent>(Prop->GetObjectPropertyValue_InContainer(CurrentActor));
	}

	// B. Fallback: Search Instance/Native
	for (UActorComponent* Comp : CurrentActor->GetComponents())
	{
		FString CleanName = Comp->GetName();
		if (CleanName.EndsWith(TEXT("_GEN_VARIABLE"))) CleanName.LeftChopInline(13);

		if (CleanName == CurrentPath)
		{
			return Cast<USceneComponent>(Comp);
		}
	}

	// C. Search SCS (for CDOs)
	if (CurrentActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(CurrentActor->GetClass()))
		{
			if (USimpleConstructionScript* SCS = BPClass->SimpleConstructionScript)
			{
				// Note: FindSCSNode uses the variable name (FName)
				if (USCS_Node* Node = SCS->FindSCSNode(FName(*CurrentPath)))
				{
					return Cast<USceneComponent>(Node->GetActualComponentTemplate(BPClass));
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

EVisibility FKzComponentSocketReferenceCustomization::GetSocketWarningVisibility() const
{
	FName CurrentSocketName;
	if (SocketNameHandle->GetValue(CurrentSocketName) != FPropertyAccess::Success)
	{
		return EVisibility::Collapsed;
	}

	// 1. If no socket specified (None), it uses the Component Pivot. Always valid.
	if (CurrentSocketName.IsNone())
	{
		return EVisibility::Collapsed;
	}

	// 2. Resolve the component first
	FName CurrentCompName;
	ComponentNameHandle->GetValue(CurrentCompName);
	USceneComponent* Comp = FindComponentByName(CurrentCompName);

	// If component is missing, the Component Warning is already shown. 
	// We hide the socket warning to reduce noise
	if (!Comp)
	{
		return EVisibility::Collapsed;
	}

	// 3. Check if socket exists on the component
	if (Comp->DoesSocketExist(CurrentSocketName))
	{
		return EVisibility::Collapsed;
	}

	// 4. Warning: Socket name not found on this component
	return EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
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
	// Check if we are in an Instance context
	bool bIsInstance = false;

	if (AActor* ContextActor = GetTargetActor())
	{
		// If the resolved actor is NOT a CDO/Archetype, we are likely in an instance
		if (!ContextActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			bIsInstance = true;
		}
	}

	if (bIsInstance && OverrideActorHandle.IsValid())
	{
		ChildBuilder.AddProperty(OverrideActorHandle.ToSharedRef());
	}

	if (RelativeLocationHandle.IsValid()) ChildBuilder.AddProperty(RelativeLocationHandle.ToSharedRef());
	if (RelativeRotationHandle.IsValid()) ChildBuilder.AddProperty(RelativeRotationHandle.ToSharedRef());
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

	// Native / Instance Components
	for (UActorComponent* Comp : Target->GetComponents())
	{
		if (Cast<USceneComponent>(Comp))
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
					if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(USceneComponent::StaticClass()))
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
	FName Val;
	ComponentNameHandle->GetValue(Val);
	return Val.IsNone() ? LOCTEXT("SelectComponent", "Select Component...") : FText::FromName(Val);
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
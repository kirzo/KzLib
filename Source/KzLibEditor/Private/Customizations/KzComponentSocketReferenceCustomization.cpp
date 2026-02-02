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

void FKzComponentSocketReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;
	OverrideActorHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, OverrideActor));
	ComponentNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, ComponentName));
	SocketNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, SocketName));
	RelativeLocationHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, RelativeLocation));
	RelativeRotationHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKzComponentSocketReference, RelativeRotation));

	// Bind callback to reset component/socket if the actor changes
	OverrideActorHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FKzComponentSocketReferenceCustomization::OnOverrideActorChanged));

	// Analyze Context (Instance vs CDO vs Non-Actor)
	bIsActorContext = false;
	bIsInstance = false;

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() > 0)
	{
		UObject* Outer = OuterObjects[0];

		// --- CASE A: The property is inside an ACTOR ---
		if (AActor* Actor = Cast<AActor>(Outer))
		{
			bIsActorContext = true;

			// It is an instance if it is NOT a Class Default Object (CDO) or Archetype
			bIsInstance = !Actor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject);
		}
		// --- CASE B: The property is inside an ACTOR COMPONENT ---
		else if (UActorComponent* Comp = Cast<UActorComponent>(Outer))
		{
			// If it's a component, it implies an Actor context eventually (Owner or BP Class)
			bIsActorContext = true;

			// Sub-Case B1: Component Instance (It has a valid Owner in the world)
			if (Comp->GetOwner())
			{
				bIsInstance = true;
			}
			// Sub-Case B2: Component Template (Inside BP Editor, no owner yet)
			else
			{
				bIsInstance = false;
			}
		}
	}

	// Build UI
	TSharedPtr<SHorizontalBox> HBox = SNew(SHorizontalBox);

	// Determine if we can use the Smart Pickers
	if (bIsActorContext)
	{
		// Component Picker
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

		// Socket Picker
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
		// Fallback: Manual Text Input (e.g. Data Assets)
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
		.MinDesiredWidth(350.0f)
		.MaxDesiredWidth(800.0f)
		[
			HBox.ToSharedRef()
		];
}

void FKzComponentSocketReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (bIsInstance && OverrideActorHandle.IsValid())
	{
		ChildBuilder.AddProperty(OverrideActorHandle.ToSharedRef());
	}

	if (RelativeLocationHandle.IsValid())
	{
		ChildBuilder.AddProperty(RelativeLocationHandle.ToSharedRef());
	}

	if (RelativeRotationHandle.IsValid())
	{
		ChildBuilder.AddProperty(RelativeRotationHandle.ToSharedRef());
	}
}

AActor* FKzComponentSocketReferenceCustomization::GetTargetActor() const
{
	// Check if Override Actor is set
	UObject* OverrideObj = nullptr;
	if (OverrideActorHandle.IsValid() && OverrideActorHandle->GetValue(OverrideObj) == FPropertyAccess::Success)
	{
		if (AActor* Override = Cast<AActor>(OverrideObj))
		{
			return Override;
		}
	}

	// Fallback to Property Context (Owner / CDO)
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	if (OuterObjects.Num() > 0)
	{
		UObject* Outer = OuterObjects[0];

		// Case A: Property is on the Actor itself (Instance or CDO)
		if (AActor* OuterActor = Cast<AActor>(Outer))
		{
			return OuterActor;
		}
		// Case B: Property is on a Component
		else if (UActorComponent* Comp = Cast<UActorComponent>(Outer))
		{
			// Sub-Case B1: Component Instance (Level) -> Return Owner
			if (AActor* Owner = Comp->GetOwner())
			{
				return Owner;
			}

			// Sub-Case B2: Component Template (Blueprint Editor)
			// The outer of a Component Template in the SCS is the BlueprintGeneratedClass
			UObject* CompOuter = Comp->GetOuter();

			if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(CompOuter))
			{
				return Cast<AActor>(BPClass->GetDefaultObject());
			}
			// Sometimes native component templates are parented to the CDO directly
			else if (AActor* CDO = Cast<AActor>(CompOuter))
			{
				return CDO;
			}
		}
	}

	return nullptr;
}

void FKzComponentSocketReferenceCustomization::OnOverrideActorChanged()
{
	// If the actor changes, the previous component name is likely invalid. Reset it.
	ComponentNameHandle->SetValue(FName(NAME_None));
	SocketNameHandle->SetValue(FName(NAME_None));
}

void FKzComponentSocketReferenceCustomization::BuildComponentList(TArray<FName>& OutNames)
{
	AActor* Target = GetTargetActor();
	if (!Target)
	{
		return;
	}

	if (Target->HasAnyFlags(RF_ClassDefaultObject))
	{
		if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(Target->GetClass()))
		{
			// Check Simple Construction Script (SCS) for components added in BP
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

	// This handles both, level instances and native components (added in c++)
	for (UActorComponent* Comp : Target->GetComponents())
	{
		if (Cast<USceneComponent>(Comp))
		{
			OutNames.Add(Comp->GetFName());
		}
	}
}

TSharedRef<SWidget> FKzComponentSocketReferenceCustomization::OnGetComponentsMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<FName> Names;
	BuildComponentList(Names);

	// Option to clear
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

	// If the selected component is the same as the current one, do nothing to preserve the socket
	if (CurrentName == InName)
	{
		return;
	}

	ComponentNameHandle->SetValue(InName);

	// Clear socket when component changes
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
	if (!Target)
	{
		return nullptr;
	}

	if (Target->HasAnyFlags(RF_ClassDefaultObject))
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

	for (UActorComponent* Comp : Target->GetComponents())
	{
		if (Comp->GetFName() == Name) return Cast<USceneComponent>(Comp);
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

		// Option to clear (Use Component Origin)
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
// Copyright 2026 kirzo

#include "Pins/SKzTypeDefGraphPin.h"
#include "Widgets/SKzTypeSelector.h"
#include "Widgets/Layout/SBox.h"
#include "ScopedTransaction.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "SKzTypeDefGraphPin"

void SKzTypeDefGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);

	if (LabelAndValue.IsValid())
	{
		LabelAndValue->SetWrapSize(TNumericLimits<float>::Max());
		LabelAndValue->SetInnerSlotPadding(FVector2D(8.0f, 0.0f));
	}
}

TSharedRef<SWidget> SKzTypeDefGraphPin::GetDefaultValueWidget()
{
	return SNew(SKzTypeSelector)
		.Visibility(this, &SKzTypeDefGraphPin::GetPickerVisibility)
		.AllowArrays(true)
		.SelectorType(SPinTypeSelector::ESelectorType::Full)
		.ValueType(this, &SKzTypeDefGraphPin::GetValueType)
		.ValueTypeObject(this, &SKzTypeDefGraphPin::GetValueTypeObject)
		.ContainerType(this, &SKzTypeDefGraphPin::GetContainerType)
		.OnTypeChanged(this, &SKzTypeDefGraphPin::OnTypeChanged);
}

EVisibility SKzTypeDefGraphPin::GetPickerVisibility() const
{
	if (!GraphPinObj || GraphPinObj->LinkedTo.Num() > 0 || GraphPinObj->bDefaultValueIsIgnored)
	{
		return EVisibility::Collapsed;
	}
	return EVisibility::Visible;
}

const FKzTypeDef& SKzTypeDefGraphPin::ReadValue() const
{
	if (!GraphPinObj)
	{
		CachedRawString.Empty();
		CachedValue = FKzTypeDef();
		return CachedValue;
	}

	// Re-parse only when the underlying default value string changed; otherwise hit the cache.
	if (GraphPinObj->DefaultValue == CachedRawString)
	{
		return CachedValue;
	}

	CachedRawString = GraphPinObj->DefaultValue;
	CachedValue = FKzTypeDef();

	if (!CachedRawString.IsEmpty())
	{
		FKzTypeDef::StaticStruct()->ImportText(*CachedRawString, &CachedValue, nullptr, PPF_None, GLog, FKzTypeDef::StaticStruct()->GetName());
	}

	return CachedValue;
}

void SKzTypeDefGraphPin::WriteValue(const FKzTypeDef& NewValue)
{
	if (!GraphPinObj)
	{
		return;
	}

	FString Exported;
	FKzTypeDef::StaticStruct()->ExportText(Exported, &NewValue, nullptr, nullptr, PPF_None, nullptr);

	if (GraphPinObj->DefaultValue.Equals(Exported))
	{
		return;
	}

	UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode();
	if (!OwningNode)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ChangeTypeDefPin", "Change Type"));
	OwningNode->Modify();

	GraphPinObj->DefaultValue = Exported;
	GraphPinObj->DefaultObject = nullptr;
	GraphPinObj->DefaultTextValue = FText::GetEmpty();

	OwningNode->PinDefaultValueChanged(GraphPinObj);

	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(OwningNode))
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

EPropertyBagPropertyType SKzTypeDefGraphPin::GetValueType() const
{
	return ReadValue().ValueType;
}

const UObject* SKzTypeDefGraphPin::GetValueTypeObject() const
{
	return ReadValue().ValueTypeObject.Get();
}

EPropertyBagContainerType SKzTypeDefGraphPin::GetContainerType() const
{
	return ReadValue().ContainerType;
}

void SKzTypeDefGraphPin::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	WriteValue(FKzTypeDef(NewContainerType, NewType, NewTypeObject));
}

#undef LOCTEXT_NAMESPACE
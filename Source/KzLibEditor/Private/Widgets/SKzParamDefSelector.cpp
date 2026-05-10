// Copyright 2026 kirzo

#include "Widgets/SKzParamDefSelector.h"
#include "Widgets/SKzTypeSelector.h"

void SKzParamDefSelector::Construct(const FArguments& InArgs)
{
	ValueAttribute = InArgs._Value;
	OnValueChangedDelegate = InArgs._OnValueChanged;

	ChildSlot
		[
			SNew(SKzTypeSelector)
				.AllowArrays(InArgs._AllowArrays)
				.ValueType_Lambda([this]() { return ValueAttribute.Get().ValueType; })
				.ValueTypeObject_Lambda([this]() -> const UObject* { return ValueAttribute.Get().ValueTypeObject.Get(); })
				.ContainerType_Lambda([this]() { return ValueAttribute.Get().ContainerType; })
				.OnTypeChanged(this, &SKzParamDefSelector::OnTypeChanged)
		];
}

void SKzParamDefSelector::OnTypeChanged(EPropertyBagPropertyType NewValueType, const UObject* NewValueTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (OnValueChangedDelegate.IsBound())
	{
		FKzParamDef NewDef;
		NewDef.Name = ValueAttribute.Get().Name;
		NewDef.ValueType = NewValueType;
		NewDef.ValueTypeObject = NewValueTypeObject;
		NewDef.ContainerType = NewContainerType;

		OnValueChangedDelegate.Execute(NewDef);
	}
}
// Copyright 2026 kirzo

#include "Widgets/SKzTypeSelector.h"
#include "KzPinTypeUtils.h"
#include "Schemas/KzParamDefSchema.h"
#include "SPinTypeSelector.h"
#include "DetailLayoutBuilder.h"

void SKzTypeSelector::Construct(const FArguments& InArgs)
{
	ValueTypeAttribute = InArgs._ValueType;
	ValueTypeObjectAttribute = InArgs._ValueTypeObject;
	ContainerTypeAttribute = InArgs._ContainerType;
	OnTypeChangedDelegate = InArgs._OnTypeChanged;

	const UKzParamDefSchema* Schema = GetDefault<UKzParamDefSchema>();

	ChildSlot
		[
			SNew(SBox)
				.MaxDesiredHeight(20.0f)
				[
					SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UKzParamDefSchema::GetKzParamTypeTree))
						.Schema(Schema)
						.TargetPinType(this, &SKzTypeSelector::GetPinType)
						.OnPinTypeChanged(this, &SKzTypeSelector::OnPinTypeChanged)
						.TypeTreeFilter(ETypeTreeFilter::None)
						.bAllowArrays(InArgs._AllowArrays)
						.SelectorType(InArgs._SelectorType)
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

FEdGraphPinType SKzTypeSelector::GetPinType() const
{
	return KzLib::Editor::PinTypeFromBagType(ValueTypeAttribute.Get(), ValueTypeObjectAttribute.Get(), ContainerTypeAttribute.Get());
}

void SKzTypeSelector::OnPinTypeChanged(const FEdGraphPinType& InPinType)
{
	if (OnTypeChangedDelegate.IsBound())
	{
		EPropertyBagPropertyType NewValueType = EPropertyBagPropertyType::None;
		const UObject* NewValueTypeObject = nullptr;
		EPropertyBagContainerType NewContainerType = EPropertyBagContainerType::None;

		KzLib::Editor::BagTypeFromPinType(InPinType, NewValueType, NewValueTypeObject, NewContainerType);

		OnTypeChangedDelegate.Execute(NewValueType, NewValueTypeObject, NewContainerType);
	}
}
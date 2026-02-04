// Copyright 2026 kirzo

#include "Widgets/SKzParamDefSelector.h"
#include "KzParamDefEditorUtils.h"
#include "Schemas/KzParamDefSchema.h"
#include "SPinTypeSelector.h"
#include "DetailLayoutBuilder.h"

void SKzParamDefSelector::Construct(const FArguments& InArgs)
{
	ValueAttribute = InArgs._Value;
	OnValueChangedDelegate = InArgs._OnValueChanged;

	const UKzParamDefSchema* Schema = GetDefault<UKzParamDefSchema>();

	ChildSlot
		[
			SNew(SBox)
				.MaxDesiredHeight(20.0f)
				[
					SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UKzParamDefSchema::GetKzParamTypeTree))
						.Schema(Schema)
						.TargetPinType(this, &SKzParamDefSelector::GetPinType)
						.OnPinTypeChanged(this, &SKzParamDefSelector::OnPinTypeChanged)
						.TypeTreeFilter(ETypeTreeFilter::None)
						.bAllowArrays(InArgs._AllowArrays)
						.Font(IDetailLayoutBuilder::GetDetailFont())
				]
		];
}

FEdGraphPinType SKzParamDefSelector::GetPinType() const
{
	// FKzParamDef -> FEdGraphPinType
	return KzLib::Editor::PinTypeFromDef(ValueAttribute.Get());
}

void SKzParamDefSelector::OnPinTypeChanged(const FEdGraphPinType& InPinType)
{
	if (OnValueChangedDelegate.IsBound())
	{
		// Preserve the existing name, update the type
		FName CurrentName = ValueAttribute.Get().Name;

		// FEdGraphPinType -> FKzParamDef
		FKzParamDef NewDef = KzLib::Editor::DefFromPinType(CurrentName, InPinType);

		OnValueChangedDelegate.Execute(NewDef);
	}
}
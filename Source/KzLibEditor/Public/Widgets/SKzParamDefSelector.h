// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Core/KzParamDef.h"

/**
 * A reusable Slate widget that displays a PinTypeSelector
 * but works directly with FKzParamDef data.
 */
class SKzParamDefSelector : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnParamDefChanged, const FKzParamDef& /*NewDef*/);

	SLATE_BEGIN_ARGS(SKzParamDefSelector)
		: _AllowArrays(true)
		{
		}
		/** The current definition value to display */
		SLATE_ATTRIBUTE(FKzParamDef, Value)

		/** Called when the user changes the type */
		SLATE_EVENT(FOnParamDefChanged, OnValueChanged)

		/** Whether to allow Array containers */
		SLATE_ARGUMENT(bool, AllowArrays)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FEdGraphPinType GetPinType() const;
	void OnPinTypeChanged(const FEdGraphPinType& InPinType);

	TAttribute<FKzParamDef> ValueAttribute;
	FOnParamDefChanged OnValueChangedDelegate;
};
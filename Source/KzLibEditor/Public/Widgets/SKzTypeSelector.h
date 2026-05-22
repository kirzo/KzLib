// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "StructUtils/PropertyBag.h"
#include "SPinTypeSelector.h"

/** A reusable Slate widget for picking a single property type. */
class KZLIBEDITOR_API SKzTypeSelector : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_ThreeParams(FOnTypeChanged, EPropertyBagPropertyType, const UObject*, EPropertyBagContainerType);

	SLATE_BEGIN_ARGS(SKzTypeSelector)
		: _AllowArrays(true)
		, _SelectorType(SPinTypeSelector::ESelectorType::Full)
		{
		}
		SLATE_ATTRIBUTE(EPropertyBagPropertyType, ValueType)
		SLATE_ATTRIBUTE(const UObject*, ValueTypeObject)
		SLATE_ATTRIBUTE(EPropertyBagContainerType, ContainerType)
		SLATE_EVENT(FOnTypeChanged, OnTypeChanged)
		SLATE_ARGUMENT(bool, AllowArrays)
		/** Layout flavor. Full = details panel style with container twiddler; Compact = single button suitable for inline graph pin display. */
		SLATE_ARGUMENT(SPinTypeSelector::ESelectorType, SelectorType)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FEdGraphPinType GetPinType() const;
	void OnPinTypeChanged(const FEdGraphPinType& InPinType);

	TAttribute<EPropertyBagPropertyType> ValueTypeAttribute;
	TAttribute<const UObject*> ValueTypeObjectAttribute;
	TAttribute<EPropertyBagContainerType> ContainerTypeAttribute;
	FOnTypeChanged OnTypeChangedDelegate;
};
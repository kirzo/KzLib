// Copyright 2026 kirzo

#include "Customizations/KzVariantCustomization.h"
#include "Core/KzVariant.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "IStructureDataProvider.h"
#include "ScopedTransaction.h"
#include "UObject/StructOnScope.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "KzVariantCustomization"

namespace KzVariantCustomization
{
	/** Shows the variants' value through their bag struct, so edits go through the variant's own handle and can be undone. */
	class FValueProvider : public IStructureDataProvider
	{
	public:
		explicit FValueProvider(const TSharedRef<IPropertyHandle>& InHandle)
			: Handle(InHandle)
		{
		}

		virtual bool IsValid() const override
		{
			return GetBaseStructure() != nullptr;
		}

		/** The bag struct all edited variants share; none while their types differ. */
		virtual const UStruct* GetBaseStructure() const override
		{
			const UStruct* Common = nullptr;
			bool bFirst = true;
			Handle->EnumerateConstRawData([&Common, &bFirst](const void* RawData, const int32, const int32)
			{
				const UStruct* Struct = RawData ? static_cast<const FKzVariant*>(RawData)->GetBagStruct() : nullptr;
				Common = bFirst || Common == Struct ? Struct : nullptr;
				bFirst = false;
				return Common != nullptr;
			});
			return Common;
		}

		virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override
		{
			TArray<UPackage*> Packages;
			Handle->GetOuterPackages(Packages);
			Handle->EnumerateRawData([&OutInstances, &Packages, ExpectedBaseStructure](void* RawData, const int32 DataIndex, const int32)
			{
				TSharedPtr<FStructOnScope> Instance;
				const FKzVariant* Variant = static_cast<const FKzVariant*>(RawData);
				if (Variant && ExpectedBaseStructure && Variant->GetBagStruct() == ExpectedBaseStructure)
				{
					Instance = MakeShared<FStructOnScope>(Variant->GetBagStruct(), Variant->GetBagMemory());
					if (Packages.IsValidIndex(DataIndex))
					{
						Instance->SetPackage(Packages[DataIndex]);
					}
				}
				OutInstances.Add(Instance);
				return true;
			});
		}

		virtual bool IsPropertyIndirection() const override
		{
			return true;
		}

		virtual uint8* GetValueBaseAddress(uint8* ParentValueAddress, const UStruct* ExpectedBaseStructure) const override
		{
			const FKzVariant* Variant = reinterpret_cast<const FKzVariant*>(ParentValueAddress);
			return Variant && ExpectedBaseStructure && Variant->GetBagStruct() == ExpectedBaseStructure ? Variant->GetBagMemory() : nullptr;
		}

	private:
		TSharedRef<IPropertyHandle> Handle;
	};

	/** The edited variant, or null on multi-edit. */
	const FKzVariant* GetVariant(const TSharedRef<IPropertyHandle>& Handle)
	{
		TArray<void*> RawData;
		Handle->AccessRawData(RawData);
		return RawData.Num() == 1 ? static_cast<const FKzVariant*>(RawData[0]) : nullptr;
	}

	void SetType(const TSharedRef<IPropertyHandle>& Handle, const TSharedPtr<IPropertyUtilities>& PropertyUtilities, EPropertyBagPropertyType NewType, const UObject* NewTypeObject)
	{
		FScopedTransaction Transaction(LOCTEXT("ChangeVariantType", "Change Variant Type"));
		Handle->NotifyPreChange();

		TArray<void*> RawData;
		Handle->AccessRawData(RawData);
		for (void* Data : RawData)
		{
			if (Data)
			{
				static_cast<FKzVariant*>(Data)->SetType(NewType, NewTypeObject);
			}
		}

		Handle->NotifyPostChange(EPropertyChangeType::ValueSet);
		Handle->NotifyFinishedChangingProperties();

		// The value row belongs to the old type's struct.
		if (PropertyUtilities.IsValid())
		{
			PropertyUtilities->RequestForceRefresh();
		}
	}
}

void FKzVariantCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedRef<SWidget> NameWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(12.0f, 0.0f, 0.0f, 0.0f)
		[
			MakeTypeSelector(PropertyHandle, CustomizationUtils.GetPropertyUtilities())
		];

	AddVariantRow(PropertyHandle, ChildBuilder, NameWidget);
}

TSharedRef<SWidget> FKzVariantCustomization::MakeTypeSelector(const TSharedRef<IPropertyHandle>& VariantHandle, const TSharedPtr<IPropertyUtilities>& PropertyUtilities)
{
	return SNew(SKzTypeSelector)
		.AllowArrays(false)
		.Visibility(FKzPropertyHandleUtils::HasMetaDataInHierarchy(VariantHandle, TEXT("FixedType")) ? EVisibility::Collapsed : EVisibility::Visible)
		.ValueType_Lambda([VariantHandle]()
		{
			const FKzVariant* Variant = KzVariantCustomization::GetVariant(VariantHandle);
			return Variant ? Variant->GetType() : EPropertyBagPropertyType::None;
		})
		.ValueTypeObject_Lambda([VariantHandle]()
		{
			const FKzVariant* Variant = KzVariantCustomization::GetVariant(VariantHandle);
			return Variant ? Variant->GetTypeObject() : nullptr;
		})
		.OnTypeChanged_Lambda([VariantHandle, PropertyUtilities](EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType)
		{
			KzVariantCustomization::SetType(VariantHandle, PropertyUtilities, NewType, NewTypeObject);
		});
}

void FKzVariantCustomization::AddVariantRow(const TSharedRef<IPropertyHandle>& VariantHandle, IDetailChildrenBuilder& ChildBuilder, const TSharedRef<SWidget>& NameWidget)
{
	const TSharedRef<KzVariantCustomization::FValueProvider> Provider = MakeShared<KzVariantCustomization::FValueProvider>(VariantHandle);
	if (!Provider->IsValid())
	{
		// No type yet, or several at once: just the name and the type selector.
		ChildBuilder.AddCustomRow(VariantHandle->GetPropertyDisplayName())
			.NameContent()
			[
				NameWidget
			];
		return;
	}

	// The bag struct holds one property, so this is the value's row.
	for (const TSharedPtr<IPropertyHandle>& ValueHandle : VariantHandle->AddChildStructure(Provider))
	{
		IDetailPropertyRow& Row = ChildBuilder.AddProperty(ValueHandle.ToSharedRef());
		TSharedPtr<SWidget> DefaultNameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FDetailWidgetRow DefaultRow;
		Row.GetDefaultWidgets(DefaultNameWidget, ValueWidget, DefaultRow);
		Row.CustomWidget(/*bShowChildren=*/true)
			.NameContent()
			.HAlign(HAlign_Fill)
			[
				NameWidget
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			[
				ValueWidget.ToSharedRef()
			];
	}
}

#undef LOCTEXT_NAMESPACE
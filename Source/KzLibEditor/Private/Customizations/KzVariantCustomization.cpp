// Copyright 2026 kirzo

#include "Customizations/KzVariantCustomization.h"
#include "Core/KzVariant.h"
#include "Widgets/SKzTypeSelector.h"
#include "Utils/KzEditorUtils.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "IStructureDataProvider.h"
#include "ScopedTransaction.h"
#include "UObject/StructOnScope.h"

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
}

void FKzVariantCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	if (FKzPropertyHandleUtils::HasMetaDataInHierarchy(PropertyHandle, TEXT("ShowOnlyInnerProperties")))
	{
		return;
	}

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SKzTypeSelector)
				.AllowArrays(false)
				.Visibility_Lambda([this]() { return FKzPropertyHandleUtils::HasMetaDataInHierarchy(StructHandle, TEXT("FixedType")) ? EVisibility::Collapsed : EVisibility::Visible; })
				.ValueType_Lambda([this]() { const FKzVariant* Variant = GetVariant(); return Variant ? Variant->GetType() : EPropertyBagPropertyType::None; })
				.ValueTypeObject_Lambda([this]() { const FKzVariant* Variant = GetVariant(); return Variant ? Variant->GetTypeObject() : nullptr; })
				.OnTypeChanged(this, &FKzVariantCustomization::OnTypeChanged)
		];
}

void FKzVariantCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	const TSharedRef<KzVariantCustomization::FValueProvider> Provider = MakeShared<KzVariantCustomization::FValueProvider>(PropertyHandle);
	if (!Provider->IsValid())
	{
		return;
	}

	for (const TSharedPtr<IPropertyHandle>& ChildHandle : PropertyHandle->AddChildStructure(Provider))
	{
		ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
	}
}

const FKzVariant* FKzVariantCustomization::GetVariant() const
{
	if (!StructHandle.IsValid())
	{
		return nullptr;
	}

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	return RawData.Num() == 1 ? static_cast<const FKzVariant*>(RawData[0]) : nullptr;
}

void FKzVariantCustomization::OnTypeChanged(EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType NewContainerType)
{
	if (!StructHandle.IsValid())
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("ChangeVariantType", "Change Variant Type"));
	StructHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	for (void* Data : RawData)
	{
		if (Data)
		{
			static_cast<FKzVariant*>(Data)->SetType(NewType, NewTypeObject);
		}
	}

	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructHandle->NotifyFinishedChangingProperties();

	// The value row belongs to the old type's struct.
	if (PropertyUtilities.IsValid())
	{
		PropertyUtilities->RequestForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
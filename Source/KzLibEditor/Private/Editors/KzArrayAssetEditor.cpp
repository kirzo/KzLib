// Copyright 2026 kirzo

#include "Editors/KzArrayAssetEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailCustomization.h"
#include "ISinglePropertyView.h"
#include "IStructureDetailsView.h"
#include "IStructureDataProvider.h"
#include "Widgets/Layout/SBox.h"
#include "UObject/StructOnScope.h"

const FName FKzArrayAssetEditor::AssetDetailsTabId(TEXT("KzArrayEditor_AssetDetails"));
const FName FKzArrayAssetEditor::ArrayStackTabId(TEXT("KzArrayEditor_ArrayStack"));
const FName FKzArrayAssetEditor::ElementDetailsTabId(TEXT("KzArrayEditor_ElementDetails"));

class FKzArrayAssetDetailCustomization : public IDetailCustomization
{
public:
	FKzArrayAssetDetailCustomization(FKzArrayAssetEditor* InEditor) : Editor(InEditor) {}

	static TSharedRef<IDetailCustomization> MakeInstance(FKzArrayAssetEditor* InEditor)
	{
		return MakeShareable(new FKzArrayAssetDetailCustomization(InEditor));
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		if (Editor)
		{
			TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(Editor->ArrayPropertyName);
			DetailBuilder.HideProperty(Handle);

			Editor->ArrayPropertyHandle = Handle;

			if (Editor->PropertyStackWidget.IsValid())
			{
				Editor->PropertyStackWidget->SetPropertyHandle(Handle);
			}
		}
	}

private:
	FKzArrayAssetEditor* Editor;
};

// Safely provides dynamic memory addresses for struct properties inside an array
class FKzStructProvider : public IStructureDataProvider
{
public:
	TSharedPtr<IPropertyHandle> StructHandle;

	FKzStructProvider(TSharedPtr<IPropertyHandle> InHandle) : StructHandle(InHandle) {}

	virtual bool IsValid() const override
	{
		return StructHandle.IsValid() && StructHandle->IsValidHandle();
	}

	virtual const UStruct* GetBaseStructure() const override
	{
		if (IsValid())
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(StructHandle->GetProperty()))
			{
				return StructProp->Struct;
			}
		}
		return nullptr;
	}

	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override
	{
		if (IsValid())
		{
			void* Data = nullptr;
			if (StructHandle->GetValueData(Data) == FPropertyAccess::Success && Data)
			{
				// Provide a non-owning struct on scope. It views the data without taking ownership.
				TSharedPtr<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(ExpectedBaseStructure, (uint8*)Data);

				// We retrieve the outer asset package and inject it into the scope.
				TArray<UObject*> OuterObjects;
				StructHandle->GetOuterObjects(OuterObjects);
				if (OuterObjects.Num() > 0 && OuterObjects[0])
				{
					StructOnScope->SetPackage(OuterObjects[0]->GetPackage());
				}

				OutInstances.Add(StructOnScope);
			}
		}
	}

	virtual bool IsPropertyIndirection() const override
	{
		return false; // False indicates we are providing the direct memory block
	}

	virtual uint8* GetValueBaseAddress(uint8* ParentValueAddress, const UStruct* ExpectedBaseStructure) const override
	{
		if (IsValid())
		{
			void* Data = nullptr;
			// Automatically fetches the valid pointer even if the array reallocates
			if (StructHandle->GetValueData(Data) == FPropertyAccess::Success)
			{
				return (uint8*)Data;
			}
		}
		return nullptr;
	}
};

TSharedRef<FKzArrayAssetEditor> FKzArrayAssetEditor::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit, FName InArrayPropertyName, FText InItemName, SKzPropertyStack::FOnGetItemDisplayName InOnGetItemDisplayName)
{
	TSharedRef<FKzArrayAssetEditor> NewEditor(new FKzArrayAssetEditor());
	if (ObjectsToEdit.Num() > 0)
	{
		if (UObject* Asset = ObjectsToEdit[0])
		{
			NewEditor->InitArrayAssetEditor(Mode, InitToolkitHost, Asset, InArrayPropertyName, InItemName, InOnGetItemDisplayName);
		}
	}
	return NewEditor;
}

void FKzArrayAssetEditor::InitArrayAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset, FName InArrayPropertyName, FText InItemName, SKzPropertyStack::FOnGetItemDisplayName InOnGetItemDisplayName)
{
	AssetToEdit = InAsset;
	ArrayPropertyName = InArrayPropertyName;
	ItemName = InItemName;
	OnGetItemDisplayNameDelegate = InOnGetItemDisplayName;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;

	AssetDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	AssetDetailsView->RegisterInstancedCustomPropertyLayout(
		AssetToEdit->GetClass(),
		FOnGetDetailCustomizationInstance::CreateLambda([this]() {
			return FKzArrayAssetDetailCustomization::MakeInstance(this);
			})
	);

	AssetDetailsView->SetObject(AssetToEdit);

	ElementDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_KzArrayEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.4f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(AssetDetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(ArrayStackTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(ElementDetailsTabId, ETabState::OpenedTab)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FName("KzArrayEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, AssetToEdit);
}

void FKzArrayAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("KzArrayEditor", "WorkspaceMenu", "Kz Array Editor"));

	InTabManager->RegisterTabSpawner(AssetDetailsTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_AssetDetails))
		.SetDisplayName(NSLOCTEXT("KzArrayEditor", "AssetDetailsTab", "Asset Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(ArrayStackTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_ArrayStack))
		.SetDisplayName(FText::Format(NSLOCTEXT("KzArrayEditor", "ArrayStackTab", "{0}s"), ItemName))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Outliner"));

	InTabManager->RegisterTabSpawner(ElementDetailsTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_ElementDetails))
		.SetDisplayName(FText::Format(NSLOCTEXT("KzArrayEditor", "ElementDetailsTab", "{0} Details"), ItemName))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Properties"));
}

void FKzArrayAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(ArrayStackTabId);
	InTabManager->UnregisterTabSpawner(ElementDetailsTabId);
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_AssetDetails(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzArrayEditor", "AssetDetailsTitle", "Asset Details"))
		[
			AssetDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_ArrayStack(const FSpawnTabArgs& Args)
{
	SAssignNew(PropertyStackWidget, SKzPropertyStack, ArrayPropertyHandle)
		.bAllowDuplicates(false)
		.ItemName(ItemName)
		.OnGetItemDisplayName(OnGetItemDisplayNameDelegate)
		.OnItemSelected(this, &FKzArrayAssetEditor::OnElementSelected);

	return SNew(SDockTab)
		.Label(FText::Format(NSLOCTEXT("KzArrayEditor", "ArrayStackTitle", "{0}s"), ItemName))
		[
			PropertyStackWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_ElementDetails(const FSpawnTabArgs& Args)
{
	SAssignNew(ElementDetailsContainer, SBox);

	return SNew(SDockTab)
		.Label(FText::Format(NSLOCTEXT("KzArrayEditor", "ElementDetailsTitle", "{0} Details"), ItemName))
		[
			ElementDetailsContainer.ToSharedRef()
		];
}

FName FKzArrayAssetEditor::GetToolkitFName() const { return FName("KzArrayAssetEditor"); }
FText FKzArrayAssetEditor::GetBaseToolkitName() const { return NSLOCTEXT("KzArrayEditor", "AppLabel", "Array Asset Editor"); }
FString FKzArrayAssetEditor::GetWorldCentricTabPrefix() const { return TEXT("ArrayAssetEditor"); }
FLinearColor FKzArrayAssetEditor::GetWorldCentricTabColorScale() const { return FLinearColor::White; }

void FKzArrayAssetEditor::OnElementSelected(TSharedPtr<IPropertyHandle> SelectedHandle)
{
	if (!ElementDetailsContainer.IsValid())
	{
		return;
	}

	// Ensure previous views are completely cleared before inspecting new handles
	ElementDetailsContainer->SetContent(SNullWidget::NullWidget);
	if (ElementDetailsView.IsValid())
	{
		ElementDetailsView->SetObject(nullptr);
	}

	if (SelectedHandle.IsValid() && SelectedHandle->IsValidHandle())
	{
		FProperty* Prop = SelectedHandle->GetProperty();

		// 1. Handle UObjects
		if (CastField<FObjectPropertyBase>(Prop))
		{
			UObject* SelectedObj = nullptr;
			SelectedHandle->GetValue(SelectedObj);

			if (ElementDetailsView.IsValid())
			{
				ElementDetailsView->SetObject(SelectedObj);
				ElementDetailsContainer->SetContent(ElementDetailsView.ToSharedRef());
			}
			return;
		}

		// 2. Handle UStructs via our custom DataProvider
		if (CastField<FStructProperty>(Prop))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
			DetailsViewArgs.bHideSelectionTip = true;

			FStructureDetailsViewArgs StructViewArgs;
			StructViewArgs.bShowObjects = true;
			StructViewArgs.bShowAssets = true;
			StructViewArgs.bShowClasses = true;
			StructViewArgs.bShowInterfaces = true;

			TSharedPtr<IStructureDataProvider> Provider = MakeShared<FKzStructProvider>(SelectedHandle);
			TSharedRef<IStructureDetailsView> StructView = PropertyEditorModule.CreateStructureProviderDetailView(DetailsViewArgs, StructViewArgs, Provider);

			// Listen to changes inside the raw struct view and manually notify the property handle
			StructView->GetOnFinishedChangingPropertiesDelegate().AddLambda([this, SelectedHandle](const FPropertyChangedEvent& Event)
				{
					if (SelectedHandle.IsValid() && SelectedHandle->IsValidHandle())
					{
						// This forces Unreal to fire the normal property chain events on the outer UObject
						SelectedHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
					}
				});

			ElementDetailsContainer->SetContent(StructView->GetWidget().ToSharedRef());
			return;
		}

		// 3. Handle Primitive Types (int, float, FName, etc.)
		ElementDetailsContainer->SetContent(
			SNew(SBox)
			.Padding(10.0f)
			.VAlign(VAlign_Top)
			[
				SelectedHandle->CreatePropertyValueWidget()
			]
		);
		return;
	}
}
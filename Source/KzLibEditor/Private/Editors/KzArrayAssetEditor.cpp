// Copyright 2026 kirzo

#include "Editors/KzArrayAssetEditor.h"
#include "Widgets/SKzPropertyStack.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"
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

#include "Widgets/SKzValidationPanel.h"
#include "Validation/KzAssetValidationUtils.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

const FName FKzArrayAssetEditor::AssetDetailsTabId(TEXT("KzArrayEditor_AssetDetails"));
const FName FKzArrayAssetEditor::ArrayStackTabId(TEXT("KzArrayEditor_ArrayStack"));
const FName FKzArrayAssetEditor::ElementDetailsTabId(TEXT("KzArrayEditor_ElementDetails"));
const FName FKzArrayAssetEditor::ValidationTabId(TEXT("KzArrayEditor_Validation"));

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

/**
 * Safely provides dynamic memory addresses for one or more struct properties so the
 * details view can multi-edit. When more than one handle is provided, the details
 * view automatically merges values and shows "Multiple Values" where they differ.
 */
class FKzStructProvider : public IStructureDataProvider
{
public:
	TArray<TSharedPtr<IPropertyHandle>> StructHandles;

	explicit FKzStructProvider(TSharedPtr<IPropertyHandle> InHandle)
	{
		if (InHandle.IsValid()) { StructHandles.Add(InHandle); }
	}

	explicit FKzStructProvider(const TArray<TSharedPtr<IPropertyHandle>>& InHandles)
		: StructHandles(InHandles)
	{
	}

	virtual bool IsValid() const override
	{
		for (const TSharedPtr<IPropertyHandle>& Handle : StructHandles)
		{
			if (Handle.IsValid() && Handle->IsValidHandle()) { return true; }
		}
		return false;
	}

	virtual const UStruct* GetBaseStructure() const override
	{
		for (const TSharedPtr<IPropertyHandle>& Handle : StructHandles)
		{
			if (Handle.IsValid() && Handle->IsValidHandle())
			{
				if (FStructProperty* StructProp = CastField<FStructProperty>(Handle->GetProperty()))
				{
					return StructProp->Struct;
				}
			}
		}
		return nullptr;
	}

	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override
	{
		for (const TSharedPtr<IPropertyHandle>& Handle : StructHandles)
		{
			if (!Handle.IsValid() || !Handle->IsValidHandle()) { continue; }

			void* Data = nullptr;
			if (Handle->GetValueData(Data) == FPropertyAccess::Success && Data)
			{
				// Provide a non-owning struct on scope. It views the data without taking ownership.
				TSharedPtr<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(ExpectedBaseStructure, (uint8*)Data);

				// We retrieve the outer asset package and inject it into the scope.
				TArray<UObject*> OuterObjects;
				Handle->GetOuterObjects(OuterObjects);
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
		// Only meaningful for single selection. Multi-edit goes through GetInstances.
		if (StructHandles.Num() == 1 && StructHandles[0].IsValid())
		{
			void* Data = nullptr;
			// Automatically fetches the valid pointer even if the array reallocates
			if (StructHandles[0]->GetValueData(Data) == FPropertyAccess::Success)
			{
				return (uint8*)Data;
			}
		}
		return nullptr;
	}
};

TSharedRef<FKzArrayAssetEditor> FKzArrayAssetEditor::CreateEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	const TArray<UObject*>& ObjectsToEdit,
	FName InArrayPropertyName,
	FText InItemName,
	TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer)
{
	TSharedRef<FKzArrayAssetEditor> NewEditor(new FKzArrayAssetEditor());
	if (ObjectsToEdit.Num() > 0)
	{
		if (UObject* Asset = ObjectsToEdit[0])
		{
			NewEditor->InitArrayAssetEditor(Mode, InitToolkitHost, Asset, InArrayPropertyName, InItemName, InRowCustomizer);
		}
	}
	return NewEditor;
}

void FKzArrayAssetEditor::InitArrayAssetEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UObject* InAsset,
	FName InArrayPropertyName,
	FText InItemName,
	TSharedPtr<FKzPropertyStackRowCustomizer> InRowCustomizer)
{
	AssetToEdit = InAsset;
	ArrayPropertyName = InArrayPropertyName;
	ItemName = InItemName;
	RowCustomizer = InRowCustomizer;

	// Notify the customizer it's been bound to a host editor.
	if (RowCustomizer.IsValid())
	{
		RowCustomizer->OnRegister(this);
	}

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
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.6f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(ElementDetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.35f)
					->AddTab(ValidationTabId, ETabState::OpenedTab)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FName("KzArrayEditorApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, AssetToEdit);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FKzArrayAssetEditor::OnClose()
{
	if (RowCustomizer.IsValid())
	{
		RowCustomizer->OnUnregister();
	}
	FAssetEditorToolkit::OnClose();
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

	InTabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_Validation))
		.SetDisplayName(NSLOCTEXT("KzArrayEditor", "ValidationTab", "Validation"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.WarningWithColor"));
}

void FKzArrayAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(ArrayStackTabId);
	InTabManager->UnregisterTabSpawner(ElementDetailsTabId);
	InTabManager->UnregisterTabSpawner(ValidationTabId);
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
		.RowCustomizer(RowCustomizer)
		.OnSelectionChanged(this, &FKzArrayAssetEditor::OnElementsSelected);

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

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_Validation(const FSpawnTabArgs& Args)
{
	SAssignNew(ValidationPanel, SKzValidationPanel)
		.OnIssueActivated(SKzValidationPanel::FOnIssueActivated::CreateSP(this, &FKzArrayAssetEditor::HandleValidationIssueActivated))
		.OnRunValidation(SKzValidationPanel::FOnRunValidation::CreateSP(this, &FKzArrayAssetEditor::HandleRunValidation));

	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzArrayEditor", "ValidationTitle", "Validation"))
		[
			ValidationPanel.ToSharedRef()
		];
}

FName FKzArrayAssetEditor::GetToolkitFName() const { return FName("KzArrayAssetEditor"); }
FText FKzArrayAssetEditor::GetBaseToolkitName() const { return NSLOCTEXT("KzArrayEditor", "AppLabel", "Array Asset Editor"); }
FString FKzArrayAssetEditor::GetWorldCentricTabPrefix() const { return TEXT("ArrayAssetEditor"); }
FLinearColor FKzArrayAssetEditor::GetWorldCentricTabColorScale() const { return FLinearColor::White; }

void FKzArrayAssetEditor::OnElementsSelected(const TArray<TSharedPtr<IPropertyHandle>>& SelectedHandles)
{
	if (!ElementDetailsContainer.IsValid()) { return; }

	// Reset state.
	ElementDetailsContainer->SetContent(SNullWidget::NullWidget);
	if (ElementDetailsView.IsValid())
	{
		ElementDetailsView->SetObject(nullptr);
	}

	if (SelectedHandles.Num() == 0) { return; }

	// Inspect the property type of the first valid handle. If subsequent handles have
	// different property types we fall back to showing only the first.
	TSharedPtr<IPropertyHandle> First;
	for (const TSharedPtr<IPropertyHandle>& Handle : SelectedHandles)
	{
		if (Handle.IsValid() && Handle->IsValidHandle()) { First = Handle; break; }
	}
	if (!First.IsValid()) { return; }

	FProperty* FirstProp = First->GetProperty();
	if (!FirstProp) { return; }

	// ---------------------------------------------------------------------------------
	// UObjects: SetObjects on the standard details view handles multi-edit (showing
	// only the common base properties when classes differ).
	// ---------------------------------------------------------------------------------
	if (CastField<FObjectPropertyBase>(FirstProp))
	{
		TArray<UObject*> Objects;
		Objects.Reserve(SelectedHandles.Num());
		for (const TSharedPtr<IPropertyHandle>& Handle : SelectedHandles)
		{
			if (!Handle.IsValid()) { continue; }
			UObject* Obj = nullptr;
			if (Handle->GetValue(Obj) == FPropertyAccess::Success && Obj)
			{
				Objects.Add(Obj);
			}
		}

		if (Objects.Num() > 0 && ElementDetailsView.IsValid())
		{
			ElementDetailsView->SetObjects(Objects);
			ElementDetailsContainer->SetContent(ElementDetailsView.ToSharedRef());
		}
		return;
	}

	// ---------------------------------------------------------------------------------
	// Structs: build a multi-handle FKzStructProvider and feed it to a structure
	// details view. The view internally shows "Multiple Values" where they differ.
	// ---------------------------------------------------------------------------------
	if (CastField<FStructProperty>(FirstProp))
	{
		// Filter to only struct handles of the same type as the first one (mixed types
		// inside a polymorphic array shouldn't happen in well-formed assets, but we
		// guard anyway).
		const UStruct* FirstStruct = CastField<FStructProperty>(FirstProp)->Struct;
		TArray<TSharedPtr<IPropertyHandle>> Compatible;
		for (const TSharedPtr<IPropertyHandle>& Handle : SelectedHandles)
		{
			if (!Handle.IsValid()) { continue; }
			if (FStructProperty* SP = CastField<FStructProperty>(Handle->GetProperty()))
			{
				if (SP->Struct == FirstStruct) { Compatible.Add(Handle); }
			}
		}
		if (Compatible.Num() == 0) { return; }

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bHideSelectionTip = true;

		FStructureDetailsViewArgs StructViewArgs;
		StructViewArgs.bShowObjects = true;
		StructViewArgs.bShowAssets = true;
		StructViewArgs.bShowClasses = true;
		StructViewArgs.bShowInterfaces = true;

		TSharedPtr<IStructureDataProvider> Provider = MakeShared<FKzStructProvider>(Compatible);
		TSharedRef<IStructureDetailsView> StructView = PropertyEditorModule.CreateStructureProviderDetailView(
			DetailsViewArgs, StructViewArgs, Provider);

		StructView->GetOnFinishedChangingPropertiesDelegate().AddLambda(
			[Compatible](const FPropertyChangedEvent& /*Event*/)
			{
				// Forward changes to each handle so undo/redo and serialization see them.
				for (const TSharedPtr<IPropertyHandle>& H : Compatible)
				{
					if (H.IsValid() && H->IsValidHandle())
					{
						H->NotifyPostChange(EPropertyChangeType::ValueSet);
					}
				}
			});

		ElementDetailsContainer->SetContent(StructView->GetWidget().ToSharedRef());
		return;
	}

	// ---------------------------------------------------------------------------------
	// Primitives: multi-edit doesn't make sense in an array; show the first handle's
	// value editor so the user can at least see something.
	// ---------------------------------------------------------------------------------
	ElementDetailsContainer->SetContent(
		SNew(SBox)
		.Padding(10.0f)
		.VAlign(VAlign_Top)
		[
			First->CreatePropertyValueWidget()
		]);
}

void FKzArrayAssetEditor::OnRunValidation()
{
	// Open the tab if it was closed and trigger a refresh.
	TSharedPtr<FTabManager> TabManagerPin = GetTabManager();
	if (TabManagerPin.IsValid())
	{
		TabManagerPin->TryInvokeTab(ValidationTabId);
	}
	if (ValidationPanel.IsValid())
	{
		ValidationPanel->RefreshIssues();
	}
}

TArray<FKzValidationIssue> FKzArrayAssetEditor::HandleRunValidation()
{
	return FKzAssetValidationUtils::RunValidation(AssetToEdit);
}

void FKzArrayAssetEditor::HandleValidationIssueActivated(const FKzValidationIssue& Issue)
{
	if (!PropertyStackWidget.IsValid()) { return; }

	// Try GUID first, fall back to array index.
	if (Issue.ContextId.IsValid())
	{
		if (PropertyStackWidget->SelectByContextId(Issue.ContextId)) { return; }
	}
	if (Issue.ContextIndex != INDEX_NONE)
	{
		PropertyStackWidget->SelectByIndex(Issue.ContextIndex);
	}
}

void FKzArrayAssetEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection("Validation");
				{
					ToolbarBuilder.AddToolBarButton(
						FUIAction(FExecuteAction::CreateSP(this, &FKzArrayAssetEditor::OnRunValidation)),
						NAME_None,
						NSLOCTEXT("KzArrayEditor", "ValidateBtn", "Validate"),
						NSLOCTEXT("KzArrayEditor", "ValidateBtnTip", "Run validation on this asset and open the Validation tab"),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
				}
				ToolbarBuilder.EndSection();
			}));

	AddToolbarExtender(Extender);
}
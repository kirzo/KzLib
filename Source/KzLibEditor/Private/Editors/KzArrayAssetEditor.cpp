// Copyright 2026 kirzo

#include "Editors/KzArrayAssetEditor.h"
#include "Widgets/SKzPropertyStack.h"
#include "Widgets/KzPropertyStackRowCustomizer.h"
#include "Widgets/SKzValidationPanel.h"
#include "Validation/KzAssetValidationUtils.h"

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
#include "Framework/MultiBox/MultiBoxBuilder.h"

const FName FKzArrayAssetEditor::AssetDetailsTabId(TEXT("KzArrayEditor_AssetDetails"));
const FName FKzArrayAssetEditor::ValidationTabId(TEXT("KzArrayEditor_Validation"));

// =======================================================================================
// IDetailCustomization that hides each configured array property from the asset details
// view (so the user only edits arrays via the tabs, not the details panel).
// =======================================================================================

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
		if (!Editor) { return; }

		for (FKzArrayAssetEditor::FTabRuntime& Runtime : Editor->TabRuntimes)
		{
			TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(Runtime.ArrayPropertyName);
			DetailBuilder.HideProperty(Handle);

			Runtime.ArrayPropertyHandle = Handle;
			if (Runtime.StackWidget.IsValid())
			{
				Runtime.StackWidget->SetPropertyHandle(Handle);
			}
		}
	}

private:
	FKzArrayAssetEditor* Editor;
};

// =======================================================================================
// Multi-handle struct provider (unchanged from previous version).
// =======================================================================================

class FKzStructProvider : public IStructureDataProvider
{
public:
	TArray<TSharedPtr<IPropertyHandle>> StructHandles;

	explicit FKzStructProvider(TSharedPtr<IPropertyHandle> InHandle)
	{
		if (InHandle.IsValid()) { StructHandles.Add(InHandle); }
	}

	explicit FKzStructProvider(const TArray<TSharedPtr<IPropertyHandle>>& InHandles)
		: StructHandles(InHandles) {
	}

	virtual bool IsValid() const override
	{
		for (const TSharedPtr<IPropertyHandle>& H : StructHandles)
		{
			if (H.IsValid() && H->IsValidHandle()) { return true; }
		}
		return false;
	}

	virtual const UStruct* GetBaseStructure() const override
	{
		for (const TSharedPtr<IPropertyHandle>& H : StructHandles)
		{
			if (H.IsValid() && H->IsValidHandle())
			{
				if (FStructProperty* SP = CastField<FStructProperty>(H->GetProperty()))
				{
					return SP->Struct;
				}
			}
		}
		return nullptr;
	}

	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances, const UStruct* ExpectedBaseStructure) const override
	{
		for (const TSharedPtr<IPropertyHandle>& H : StructHandles)
		{
			if (!H.IsValid() || !H->IsValidHandle()) { continue; }
			void* Data = nullptr;
			if (H->GetValueData(Data) == FPropertyAccess::Success && Data)
			{
				TSharedPtr<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(ExpectedBaseStructure, (uint8*)Data);
				TArray<UObject*> Outers;
				H->GetOuterObjects(Outers);
				if (Outers.Num() > 0 && Outers[0]) { StructOnScope->SetPackage(Outers[0]->GetPackage()); }
				OutInstances.Add(StructOnScope);
			}
		}
	}

	virtual bool IsPropertyIndirection() const override { return false; }

	virtual uint8* GetValueBaseAddress(uint8* /*ParentValueAddress*/, const UStruct* /*ExpectedBaseStructure*/) const override
	{
		if (StructHandles.Num() == 1 && StructHandles[0].IsValid())
		{
			void* Data = nullptr;
			if (StructHandles[0]->GetValueData(Data) == FPropertyAccess::Success) { return (uint8*)Data; }
		}
		return nullptr;
	}
};

// =======================================================================================
// Editor lifecycle
// =======================================================================================

TSharedRef<FKzArrayAssetEditor> FKzArrayAssetEditor::CreateEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	const TArray<UObject*>& ObjectsToEdit,
	const TArray<FKzArrayEditorTabConfig>& InTabs)
{
	TSharedRef<FKzArrayAssetEditor> NewEditor(new FKzArrayAssetEditor());
	if (ObjectsToEdit.Num() > 0)
	{
		if (UObject* Asset = ObjectsToEdit[0])
		{
			NewEditor->InitArrayAssetEditor(Mode, InitToolkitHost, Asset, InTabs);
		}
	}
	return NewEditor;
}

void FKzArrayAssetEditor::InitArrayAssetEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UObject* InAsset,
	const TArray<FKzArrayEditorTabConfig>& InTabs)
{
	AssetToEdit = InAsset;
	Tabs = InTabs;

	// Build per-tab runtime state.
	TabRuntimes.Reset();
	TabRuntimes.Reserve(Tabs.Num());
	for (int32 i = 0; i < Tabs.Num(); ++i)
	{
		FTabRuntime Runtime;
		Runtime.ArrayPropertyName = Tabs[i].ArrayPropertyName;
		Runtime.ItemName = Tabs[i].ItemName;
		Runtime.Customizer = Tabs[i].RowCustomizer;
		Runtime.TabId = MakeArrayStackTabId(i);
		TabRuntimes.Add(MoveTemp(Runtime));

		// Notify customizers of the host editor.
		if (Runtime.Customizer.IsValid())
		{
			Runtime.Customizer->OnRegister(this);
		}
	}

	// Asset Details view (shared).
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;

	AssetDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	AssetDetailsView->RegisterInstancedCustomPropertyLayout(
		AssetToEdit->GetClass(),
		FOnGetDetailCustomizationInstance::CreateLambda([this]()
			{ return FKzArrayAssetDetailCustomization::MakeInstance(this); }));
	AssetDetailsView->SetObject(AssetToEdit);

	ElementDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// Build layout: each array stack on the left in its own tab, shared element/details
	// + validation on the right.
	TSharedRef<FTabManager::FStack> LeftStack = FTabManager::NewStack()->SetSizeCoefficient(0.4f);
	for (const FTabRuntime& Runtime : TabRuntimes)
	{
		LeftStack->AddTab(Runtime.TabId, ETabState::OpenedTab);
	}
	// Make the first tab the foreground one.
	if (TabRuntimes.Num() > 0)
	{
		LeftStack->SetForegroundTab(TabRuntimes[0].TabId);
	}

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_KzArrayEditor_Layout_v4")
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
				->Split(LeftStack)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.6f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(GetElementDetailsTabId(), ETabState::OpenedTab)
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
	InitAssetEditor(Mode, InitToolkitHost, FName("KzArrayEditorApp"), Layout,
		bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, AssetToEdit);

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

// =======================================================================================
// Tab registration
// =======================================================================================

void FKzArrayAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		NSLOCTEXT("KzArrayEditor", "WorkspaceMenu", "Kz Array Editor"));

	InTabManager->RegisterTabSpawner(AssetDetailsTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_AssetDetails))
		.SetDisplayName(NSLOCTEXT("KzArrayEditor", "AssetDetailsTab", "Asset Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	const FName ElementDetailsTabId = GetElementDetailsTabId();
	InTabManager->RegisterTabSpawner(ElementDetailsTabId, FOnSpawnTab::CreateSP(
		this, &FKzArrayAssetEditor::SpawnTab_ElementDetails, ElementDetailsTabId))
		.SetDisplayName(NSLOCTEXT("KzArrayEditor", "ElementDetailsTab", "Element Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Properties"));

	for (int32 i = 0; i < TabRuntimes.Num(); ++i)
	{
		const FTabRuntime& Runtime = TabRuntimes[i];
		const FText PluralName = Tabs[i].GetPluralItemName();

		InTabManager->RegisterTabSpawner(Runtime.TabId,
			FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_ArrayStack, i))
			.SetDisplayName(PluralName)
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Outliner"));
	}

	InTabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateSP(this, &FKzArrayAssetEditor::SpawnTab_Validation))
		.SetDisplayName(NSLOCTEXT("KzArrayEditor", "ValidationTab", "Validation"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.WarningWithColor"));
}

void FKzArrayAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(AssetDetailsTabId);
	InTabManager->UnregisterTabSpawner(GetElementDetailsTabId());
	InTabManager->UnregisterTabSpawner(ValidationTabId);

	for (const FTabRuntime& Runtime : TabRuntimes)
	{
		InTabManager->UnregisterTabSpawner(Runtime.TabId);
	}
}

// =======================================================================================
// Tab spawners
// =======================================================================================

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_AssetDetails(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzArrayEditor", "AssetDetailsTitle", "Asset Details"))
		[
			AssetDetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_ElementDetails(const FSpawnTabArgs& /*Args*/, FName /*ElementDetailsTabId*/)
{
	SAssignNew(ElementDetailsContainer, SBox);

	return SNew(SDockTab)
		.Label(NSLOCTEXT("KzArrayEditor", "ElementDetailsTitle", "Element Details"))
		[
			ElementDetailsContainer.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_ArrayStack(const FSpawnTabArgs& /*Args*/, int32 TabIndex)
{
	check(TabRuntimes.IsValidIndex(TabIndex));
	FTabRuntime& Runtime = TabRuntimes[TabIndex];

	SAssignNew(Runtime.StackWidget, SKzPropertyStack, Runtime.ArrayPropertyHandle)
		.bAllowDuplicates(false)
		.ItemName(Runtime.ItemName)
		.ItemNamePlural(Tabs[TabIndex].GetPluralItemName())
		.RowCustomizer(Runtime.Customizer)
		.OnSelectionChanged(this, &FKzArrayAssetEditor::OnElementsSelected);

	const FText PluralLabel = Tabs[TabIndex].GetPluralItemName();

	return SNew(SDockTab)
		.Label(PluralLabel)
		.OnTabActivated(SDockTab::FOnTabActivatedCallback::CreateSP(
			this, &FKzArrayAssetEditor::OnArrayStackTabActivated, TabIndex))
		[
			Runtime.StackWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FKzArrayAssetEditor::SpawnTab_Validation(const FSpawnTabArgs& /*Args*/)
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

// =======================================================================================
// Identity
// =======================================================================================

FName FKzArrayAssetEditor::GetToolkitFName() const { return FName("KzArrayAssetEditor"); }
FText FKzArrayAssetEditor::GetBaseToolkitName() const { return NSLOCTEXT("KzArrayEditor", "AppLabel", "Array Asset Editor"); }
FString FKzArrayAssetEditor::GetWorldCentricTabPrefix() const { return TEXT("ArrayAssetEditor"); }
FLinearColor FKzArrayAssetEditor::GetWorldCentricTabColorScale() const { return FLinearColor::White; }

void FKzArrayAssetEditor::OnClose()
{
	for (FTabRuntime& Runtime : TabRuntimes)
	{
		if (Runtime.Customizer.IsValid())
		{
			Runtime.Customizer->OnUnregister();
		}
	}
	FAssetEditorToolkit::OnClose();
}

// =======================================================================================
// Selection -> Element Details
// =======================================================================================

void FKzArrayAssetEditor::OnElementsSelected(const TArray<TSharedPtr<IPropertyHandle>>& SelectedHandles)
{
	if (!ElementDetailsContainer.IsValid()) { return; }

	ElementDetailsContainer->SetContent(SNullWidget::NullWidget);
	if (ElementDetailsView.IsValid())
	{
		ElementDetailsView->SetObject(nullptr);
	}

	if (SelectedHandles.Num() == 0) { return; }

	TSharedPtr<IPropertyHandle> First;
	for (const TSharedPtr<IPropertyHandle>& Handle : SelectedHandles)
	{
		if (Handle.IsValid() && Handle->IsValidHandle()) { First = Handle; break; }
	}
	if (!First.IsValid()) { return; }

	FProperty* FirstProp = First->GetProperty();
	if (!FirstProp) { return; }

	if (CastField<FObjectPropertyBase>(FirstProp))
	{
		TArray<UObject*> Objects;
		Objects.Reserve(SelectedHandles.Num());
		for (const TSharedPtr<IPropertyHandle>& Handle : SelectedHandles)
		{
			if (!Handle.IsValid()) { continue; }
			UObject* Obj = nullptr;
			if (Handle->GetValue(Obj) == FPropertyAccess::Success && Obj) { Objects.Add(Obj); }
		}
		if (Objects.Num() > 0 && ElementDetailsView.IsValid())
		{
			ElementDetailsView->SetObjects(Objects);
			ElementDetailsContainer->SetContent(ElementDetailsView.ToSharedRef());
		}
		return;
	}

	if (CastField<FStructProperty>(FirstProp))
	{
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

	ElementDetailsContainer->SetContent(
		SNew(SBox).Padding(10.0f).VAlign(VAlign_Top)
		[
			First->CreatePropertyValueWidget()
		]);
}

void FKzArrayAssetEditor::OnArrayStackTabActivated(TSharedRef<SDockTab> /*ActivatedTab*/, ETabActivationCause Cause, int32 TabIndex)
{
	// Only react to user-driven activations. Programmatic ones (e.g. layout restore at
	// startup) shouldn't fight against the user's explicit selection in another tab.
	if (Cause != ETabActivationCause::UserClickedOnTab) { return; }
	if (!TabRuntimes.IsValidIndex(TabIndex)) { return; }

	const FTabRuntime& Runtime = TabRuntimes[TabIndex];
	if (!Runtime.StackWidget.IsValid()) { return; }

	// Push the now-active tab's selection into the shared Element Details panel.
	const TArray<TSharedPtr<IPropertyHandle>> Selection = Runtime.StackWidget->GetSelectedHandles();
	OnElementsSelected(Selection);
}

// =======================================================================================
// Validation
// =======================================================================================

void FKzArrayAssetEditor::OnRunValidation()
{
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
	// Try each tab's customizer to resolve the GUID, falling back to index.
	if (Issue.ContextId.IsValid())
	{
		for (const FTabRuntime& Runtime : TabRuntimes)
		{
			if (Runtime.StackWidget.IsValid() && Runtime.StackWidget->SelectByContextId(Issue.ContextId))
			{
				return;
			}
		}
	}

	if (Issue.ContextIndex != INDEX_NONE)
	{
		// Without further metadata we don't know which tab the index belongs to. Try
		// each in order; the first stack with a matching index wins.
		for (const FTabRuntime& Runtime : TabRuntimes)
		{
			if (Runtime.StackWidget.IsValid() && Runtime.StackWidget->SelectByIndex(Issue.ContextIndex))
			{
				return;
			}
		}
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
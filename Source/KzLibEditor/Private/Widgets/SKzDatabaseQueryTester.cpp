// Copyright 2026 kirzo

#include "Widgets/SKzDatabaseQueryTester.h"
#include "Core/KzDatabaseAsset.h"
#include "Core/KzDatabaseOwner.h"
#include "Widgets/SKzTypeSelector.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataAsset.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "IStructureDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "Styling/CoreStyle.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectHash.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SKzDatabaseQueryTester"

const FName SKzDatabaseQueryTester::TabId(TEXT("KzDatabaseQueryTest"));

namespace KzDatabaseQueryTester
{
	const FName SideColumn(TEXT("Side"));
	const FName TableColumn(TEXT("Table"));
	const FName RowColumn(TEXT("Row"));
	const FName TagsColumn(TEXT("Tags"));
	const FName ResultColumn(TEXT("Result"));

	const IKzDatabaseOwner* AsOwner(const UObject* Object)
	{
		return Cast<IKzDatabaseOwner>(Object);
	}

	/** The first database owner a picked asset holds: a Blueprint's components, made in C++ or in the Blueprint or a parent one, or a data asset's subobjects. */
	const IKzDatabaseOwner* FindOwner(const UObject* Source)
	{
		TArray<UObject*> Subobjects;
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Source))
		{
			const UClass* Class = Blueprint->GeneratedClass;
			if (const UObject* Defaults = Class ? Class->GetDefaultObject() : nullptr)
			{
				GetObjectsWithOuter(Defaults, Subobjects, /*bIncludeNestedObjects=*/false);
			}
			for (const UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class); BlueprintClass; BlueprintClass = Cast<UBlueprintGeneratedClass>(BlueprintClass->GetSuperClass()))
			{
				if (BlueprintClass->SimpleConstructionScript)
				{
					for (const USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
					{
						if (Node && Node->ComponentTemplate)
						{
							Subobjects.Add(Node->ComponentTemplate);
						}
					}
				}
			}
		}
		else if (const IKzDatabaseOwner* Owner = AsOwner(Source))
		{
			return Owner;
		}
		else
		{
			GetObjectsWithOuter(Source, Subobjects, /*bIncludeNestedObjects=*/true);
		}

		for (const UObject* Subobject : Subobjects)
		{
			if (const IKzDatabaseOwner* Owner = AsOwner(Subobject))
			{
				return Owner;
			}
		}
		return nullptr;
	}

	/** Every table of a set, parents included, in the order a query walks them. */
	void ForEachTable(const FKzDatabaseSet& Tables, TFunctionRef<void(const FKzDatabase&, const FString&)> Visit)
	{
		for (int32 Index = 0; Index < Tables.Tables.Num(); ++Index)
		{
			Visit(Tables.Tables[Index], FString::Printf(TEXT("Inline %d"), Index + 1));
		}

		for (const UKzDatabaseAsset* Asset : Tables.Databases)
		{
			TSet<const UKzDatabaseAsset*> Visited;
			for (const UKzDatabaseAsset* Table = Asset; Table && !Visited.Contains(Table); Table = Table->ParentDatabase)
			{
				Visited.Add(Table);
				Visit(Table->Database, Table->GetName());
			}
		}
	}

	FText SideLabel(const SKzDatabaseQueryTester::FResult& Result)
	{
		return Result.bUser ? LOCTEXT("User", "User") : LOCTEXT("Object", "Object");
	}

	FText RowLabel(const SKzDatabaseQueryTester::FResult& Result)
	{
		return Result.ID.IsNone() ? FText::AsNumber(Result.Index + 1) : FText::Format(LOCTEXT("RowWithId", "{0} ({1})"), FText::AsNumber(Result.Index + 1), FText::FromName(Result.ID));
	}

	FText Describe(const SKzDatabaseQueryTester::FResult& Result)
	{
		switch (Result.Match)
		{
		case EKzDatabaseMatch::Candidate:
			return FText::Format(Result.bWinner ? LOCTEXT("Wins", "Wins, score {0}") : LOCTEXT("Candidate", "Candidate, score {0}"), Result.Score);
		case EKzDatabaseMatch::MissingRequiredTag:
			return LOCTEXT("MissingRequired", "Lacks a required tag");
		case EKzDatabaseMatch::HasIgnoredTag:
			return LOCTEXT("HasIgnored", "Has an ignored tag");
		case EKzDatabaseMatch::FailsTagQuery:
			return LOCTEXT("FailsTagQuery", "Fails the tag query");
		case EKzDatabaseMatch::UncoveredTag:
			return FText::Format(LOCTEXT("Uncovered", "{0} is neither asked for nor in the context"), FText::FromName(Result.UncoveredTag.GetTagName()));
		}
		return FText::GetEmpty();
	}

	class SResultRow : public SMultiColumnTableRow<TSharedPtr<SKzDatabaseQueryTester::FResult>>
	{
	public:
		SLATE_BEGIN_ARGS(SResultRow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, const TSharedPtr<SKzDatabaseQueryTester::FResult>& InResult)
		{
			Result = InResult;
			FSuperRowType::Construct(FSuperRowType::FArguments().Padding(FMargin(0.0f, 2.0f)), OwnerTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
		{
			const FText Text = Column == SideColumn ? SideLabel(*Result)
				: Column == TableColumn ? FText::FromString(Result->Table)
				: Column == RowColumn ? RowLabel(*Result)
				: Column == TagsColumn ? FText::FromString(Result->Tags)
				: Describe(*Result);

			// Rejected rows are dimmed, so the winner and the candidates it beat stand out.
			return SNew(STextBlock)
				.Text(Text)
				.Font(FCoreStyle::GetDefaultFontStyle(Result->bWinner ? "Bold" : "Regular", 9))
				.ColorAndOpacity(Result->Match == EKzDatabaseMatch::Candidate ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground());
		}

	private:
		TSharedPtr<SKzDatabaseQueryTester::FResult> Result;
	};
}

void SKzDatabaseQueryTester::Construct(const FArguments& InArgs)
{
	ObjectSide.Source = InArgs._Object;
	const bool bObjectFixed = InArgs._Object != nullptr;
	Query = MakeShared<FStructOnScope>(FKzDatabaseQuery::StaticStruct());

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = false;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	QueryView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateStructureDetailView(ViewArgs, FStructureDetailsViewArgs(), Query);
	QueryView->GetOnFinishedChangingPropertiesDelegate().AddSPLambda(this, [this](const FPropertyChangedEvent&) { Refresh(); });

	PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SKzDatabaseQueryTester::HandleObjectPropertyChanged);

	auto MakeField = [](const FText& Label, const FText& ToolTip, const TSharedRef<SWidget>& Field) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			.ToolTipText(ToolTip)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(56.0f)
				[
					SNew(STextBlock).Text(Label)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				Field
			];
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 6.0f, 8.0f, 2.0f))
		[
			SNew(SBox)
			.Visibility(bObjectFixed ? EVisibility::Collapsed : EVisibility::Visible)
			[
				MakeField(LOCTEXT("ObjectLabel", "Object"), LOCTEXT("ObjectTooltip", "What the query is about: a table, a Blueprint with a database component, or an asset holding one, such as an item with a Database fragment. Its tables answer first."), MakeSourcePicker(ObjectSide))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 2.0f))
		[
			MakeField(LOCTEXT("UserLabel", "User"), LOCTEXT("UserTooltip", "Who asks, such as a character Blueprint: its rows override the object's only when more specific. Its state goes in the query's Context Tags."), MakeSourcePicker(UserSide))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 2.0f, 8.0f, 6.0f))
		[
			MakeField(LOCTEXT("TypeLabel", "Type"), LOCTEXT("TypeTooltip", "The type of value asked for: only tables of this type answer."),
				SNew(SBox)
				.HAlign(HAlign_Left)
				[
					SNew(SKzTypeSelector)
					.AllowArrays(false)
					.ValueType_Lambda([this]() { return Type.ValueType; })
					.ValueTypeObject_Lambda([this]() { return Type.ValueTypeObject.Get(); })
					.OnTypeChanged_Lambda([this](EPropertyBagPropertyType NewType, const UObject* NewTypeObject, EPropertyBagContainerType)
					{
						Type = FKzTypeDef(EPropertyBagContainerType::None, NewType, NewTypeObject);
						Refresh();
					})
				])
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				QueryView->GetWidget().ToSharedRef()
			]
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() { return Summary; })
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FResult>>)
					.ListItemsSource(&Results)
					.OnGenerateRow(this, &SKzDatabaseQueryTester::OnGenerateRow)
					.SelectionMode(ESelectionMode::None)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(KzDatabaseQueryTester::SideColumn).DefaultLabel(LOCTEXT("SideColumn", "Side")).FillWidth(0.1f)
						+ SHeaderRow::Column(KzDatabaseQueryTester::TableColumn).DefaultLabel(LOCTEXT("TableColumn", "Table")).FillWidth(0.18f)
						+ SHeaderRow::Column(KzDatabaseQueryTester::RowColumn).DefaultLabel(LOCTEXT("RowColumn", "Row")).FillWidth(0.12f)
						+ SHeaderRow::Column(KzDatabaseQueryTester::TagsColumn).DefaultLabel(LOCTEXT("TagsColumn", "Tags")).FillWidth(0.3f)
						+ SHeaderRow::Column(KzDatabaseQueryTester::ResultColumn).DefaultLabel(LOCTEXT("ResultColumn", "Result")).FillWidth(0.3f)
					)
				]
			]
		]
	];

	Refresh();
}

SKzDatabaseQueryTester::~SKzDatabaseQueryTester()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
}

TSharedRef<SWidget> SKzDatabaseQueryTester::MakeSourcePicker(FSide& Side)
{
	return SNew(SObjectPropertyEntryBox)
		.AllowedClass(UObject::StaticClass())
		.DisplayThumbnail(false)
		.AllowCreate(false)
		.ObjectPath_Lambda([&Side]() { return Side.Source.IsValid() ? Side.Source->GetPathName() : FString(); })
		.OnShouldFilterAsset_Lambda([](const FAssetData& AssetData)
		{
			return !AssetData.IsInstanceOf<UKzDatabaseAsset>() && !AssetData.IsInstanceOf<UBlueprint>() && !AssetData.IsInstanceOf<UDataAsset>();
		})
		.OnObjectChanged_Lambda([this, &Side](const FAssetData& AssetData)
		{
			Side.Source = AssetData.GetAsset();
			Type = FKzTypeDef();
			Refresh();
		});
}

const FKzDatabaseSet* SKzDatabaseQueryTester::GetTables(FSide& Side, FGameplayTagContainer& OutTags)
{
	UObject* Source = Side.Source.Get();
	if (UKzDatabaseAsset* Asset = Cast<UKzDatabaseAsset>(Source))
	{
		Side.AssetTables.Databases = { Asset };
		return &Side.AssetTables;
	}

	const IKzDatabaseOwner* Owner = Source ? KzDatabaseQueryTester::FindOwner(Source) : nullptr;
	if (!Owner)
	{
		return nullptr;
	}
	Owner->AppendDatabaseTags(OutTags);
	return &Owner->GetDatabaseSet();
}

void SKzDatabaseQueryTester::Refresh()
{
	Results.Reset();

	FGameplayTagContainer ObjectTags;
	FGameplayTagContainer UserTags;
	const FKzDatabaseSet* ObjectTables = GetTables(ObjectSide, ObjectTags);
	const FKzDatabaseSet* UserTables = GetTables(UserSide, UserTags);

	for (const FKzDatabaseSet* Tables : { ObjectTables, UserTables })
	{
		if (Tables && !Type.IsValid())
		{
			KzDatabaseQueryTester::ForEachTable(*Tables, [this](const FKzDatabase& Table, const FString&)
			{
				if (!Type.IsValid())
				{
					Type = Table.Type;
				}
			});
		}
	}

	const FKzDatabaseQuery& BaseQuery = *reinterpret_cast<const FKzDatabaseQuery*>(Query->GetStructMemory());
	bool bFromUser = false;
	const FKzDatabaseItem* Winner = Type.IsValid() ? FKzDatabaseSet::Resolve(Type, BaseQuery, ObjectTables, ObjectTags, UserTables, UserTags, &bFromUser) : nullptr;

	// The rows are judged in the context the rule builds: the query's tags plus both sides'.
	FKzDatabaseQuery ContextQuery = BaseQuery;
	ContextQuery.ContextTags.AppendTags(ObjectTags);
	ContextQuery.ContextTags.AppendTags(UserTags);
	if (ObjectTables)
	{
		AddRows(*ObjectTables, false, ContextQuery, Winner);
	}
	if (UserTables)
	{
		AddRows(*UserTables, true, ContextQuery, Winner);
	}

	const TSharedPtr<FResult>* WinnerRow = Results.FindByPredicate([](const TSharedPtr<FResult>& Result) { return Result->bWinner; });
	if (!ObjectSide.Source.IsValid() && !UserSide.Source.IsValid())
	{
		Summary = LOCTEXT("PickSides", "Pick an object, a user, or both.");
	}
	else if (!ObjectTables && !UserTables)
	{
		Summary = LOCTEXT("NoTables", "Neither side holds database tables.");
	}
	else if (!Type.IsValid())
	{
		Summary = LOCTEXT("NoType", "No table has a type yet.");
	}
	else if (!WinnerRow)
	{
		Summary = LOCTEXT("NoWinner", "No row matches the query: whoever asks falls back to its own default.");
	}
	else
	{
		const FResult& Best = **WinnerRow;
		Summary = FText::Format(bFromUser ? LOCTEXT("UserWins", "The user's row {0} of {1} wins with score {2}, more specific than anything of the object's.") : LOCTEXT("ObjectWins", "The object's row {0} of {1} wins with score {2}; the user overrides it only with a more specific row."),
			KzDatabaseQueryTester::RowLabel(Best), FText::FromString(Best.Table), Best.Score);
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SKzDatabaseQueryTester::AddRows(const FKzDatabaseSet& Tables, bool bUser, const FKzDatabaseQuery& ContextQuery, const FKzDatabaseItem* Winner)
{
	KzDatabaseQueryTester::ForEachTable(Tables, [&](const FKzDatabase& Table, const FString& Label)
	{
		if (Table.Type != Type)
		{
			return;
		}

		for (int32 Index = 0; Index < Table.Items.Num(); ++Index)
		{
			const FKzDatabaseItem& Item = Table.Items[Index];
			const TSharedPtr<FResult> Result = MakeShared<FResult>();
			Result->bUser = bUser;
			Result->Table = Label;
			Result->Index = Index;
			Result->ID = Item.ID;
			Result->Tags = Item.Tags.ToStringSimple();
			Result->Match = ContextQuery.Evaluate(Item.Tags, &Result->UncoveredTag);
			Result->Score = ContextQuery.CalculateScore(Item.Tags);
			Result->bWinner = &Item == Winner;
			Results.Add(Result);
		}
	});
}

void SKzDatabaseQueryTester::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// Edits of any table, owner or picked asset may change the answer; they are rare enough to just refresh.
	if (Object && (Object->IsA<UKzDatabaseAsset>() || KzDatabaseQueryTester::AsOwner(Object) || Object == ObjectSide.Source.Get() || Object == UserSide.Source.Get()))
	{
		Refresh();
	}
}

TSharedRef<ITableRow> SKzDatabaseQueryTester::OnGenerateRow(TSharedPtr<FResult> Result, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(KzDatabaseQueryTester::SResultRow, OwnerTable, Result);
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "Widgets/SKzValidationPanel.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "KzValidationPanel"

void SKzValidationPanel::Construct(const FArguments& InArgs)
{
	OnIssueActivatedDelegate = InArgs._OnIssueActivated;
	OnRunValidationDelegate = InArgs._OnRunValidation;

	// All severities enabled by default.
	EnabledSeverities.Add(EKzValidationSeverity::Info);
	EnabledSeverities.Add(EKzValidationSeverity::Warning);
	EnabledSeverities.Add(EKzValidationSeverity::Error);

	ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.f)
				[
					SNew(SVerticalBox)

						// Toolbar row: Run button + filter combo + summary text.
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
								[
									SNew(SButton)
										.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
										.OnClicked_Lambda([this]() { RefreshIssues(); return FReply::Handled(); })
										.ToolTipText(LOCTEXT("RunTip", "Run validation on this asset"))
										.Content()
										[
											SNew(SHorizontalBox)
												+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
												[
													SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
														[
															SNew(SImage).Image(FAppStyle::Get().GetBrush("Icons.Refresh"))
														]
												]
												+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
												[
													SNew(STextBlock).Text(LOCTEXT("Validate", "Validate"))
												]
										]
								]

							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Center)
								[
									SAssignNew(FilterButton, SComboButton)
										.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
										.HasDownArrow(false)
										.ContentPadding(FMargin(2.f))
										.OnGetMenuContent(this, &SKzValidationPanel::BuildSeverityFilterMenu)
										.ToolTipText_Lambda([this]()
											{
												const int32 N = CountActiveFilters();
												return N < 3
													? FText::Format(LOCTEXT("FilterTipActive", "Severity filter ({0} of 3 active)"), FText::AsNumber(N))
													: LOCTEXT("FilterTipAll", "Severity filter (all enabled)");
											})
										.ButtonContent()
										[
											SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
												[
													SNew(SImage)
														.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
														.ColorAndOpacity_Lambda([this]()
															{
																return CountActiveFilters() < 3
																	? FSlateColor(FLinearColor(0.20f, 0.55f, 1.00f))
																	: FSlateColor::UseForeground();
															})
												]
										]
								]

							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text_Lambda([this]()
											{
												const int32 Total = AllIssues.Num();
												const int32 Visible = VisibleIssues.Num();
												if (Total == 0) { return LOCTEXT("NoIssues", "No issues."); }
												if (Visible == Total) { return FText::Format(LOCTEXT("IssueCount", "{0} issue(s)"), FText::AsNumber(Total)); }
												return FText::Format(LOCTEXT("IssueCountFiltered", "{0} of {1} shown"), FText::AsNumber(Visible), FText::AsNumber(Total));
											})
										.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
						]

					// Issue list.
					+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SAssignNew(ListView, SListView<FIssuePtr>)
								.ListItemsSource(&VisibleIssues)
								.OnGenerateRow(this, &SKzValidationPanel::OnGenerateRow)
								.OnMouseButtonClick(this, &SKzValidationPanel::OnRowClicked)
								.SelectionMode(ESelectionMode::Single)
						]
				]
		];
}

void SKzValidationPanel::RefreshIssues()
{
	if (OnRunValidationDelegate.IsBound())
	{
		SetIssues(OnRunValidationDelegate.Execute());
	}
}

void SKzValidationPanel::SetIssues(const TArray<FKzValidationIssue>& InIssues)
{
	AllIssues.Reset();
	AllIssues.Reserve(InIssues.Num());
	for (const FKzValidationIssue& Issue : InIssues)
	{
		AllIssues.Add(MakeShared<FKzValidationIssue>(Issue));
	}
	RebuildVisibleIssues();
}

void SKzValidationPanel::RebuildVisibleIssues()
{
	VisibleIssues.Reset();
	VisibleIssues.Reserve(AllIssues.Num());
	for (const FIssuePtr& Issue : AllIssues)
	{
		if (Issue.IsValid() && EnabledSeverities.Contains(Issue->Severity))
		{
			VisibleIssues.Add(Issue);
		}
	}
	if (ListView.IsValid()) { ListView->RequestListRefresh(); }
}

int32 SKzValidationPanel::CountActiveFilters() const
{
	return EnabledSeverities.Num();
}

TSharedRef<SWidget> SKzValidationPanel::BuildSeverityFilterMenu()
{
	FMenuBuilder MenuBuilder(/*bShouldCloseAfter=*/false, nullptr);

	MenuBuilder.BeginSection("Severity", LOCTEXT("SeveritySection", "Severity"));
	const EKzValidationSeverity Levels[] = { EKzValidationSeverity::Error, EKzValidationSeverity::Warning, EKzValidationSeverity::Info };
	for (EKzValidationSeverity Level : Levels)
	{
		MenuBuilder.AddMenuEntry(
			GetSeverityLabel(Level),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, Level]()
					{
						if (EnabledSeverities.Contains(Level)) { EnabledSeverities.Remove(Level); }
						else { EnabledSeverities.Add(Level); }
						RebuildVisibleIssues();
					}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, Level]() { return EnabledSeverities.Contains(Level); })),
			NAME_None,
			EUserInterfaceActionType::ToggleButton);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SKzValidationPanel::OnGenerateRow(FIssuePtr Item, const TSharedRef<STableViewBase>& Owner)
{
	if (!Item.IsValid())
	{
		return SNew(STableRow<FIssuePtr>, Owner);
	}

	return SNew(STableRow<FIssuePtr>, Owner)
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[
					SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
						[
							SNew(SImage)
								.Image(GetSeverityBrush(Item->Severity))
								.ColorAndOpacity(GetSeverityColor(Item->Severity))
						]
				]

				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Item->Message).AutoWrapText(true)
				]

				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromName(Item->ValidatorId))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Font(FAppStyle::GetFontStyle("PropertyWindow.ItalicFont"))
				]
		];
}

void SKzValidationPanel::OnRowClicked(FIssuePtr Item)
{
	if (Item.IsValid() && OnIssueActivatedDelegate.IsBound())
	{
		OnIssueActivatedDelegate.Execute(*Item);
	}
}

const FSlateBrush* SKzValidationPanel::GetSeverityBrush(EKzValidationSeverity Severity) const
{
	switch (Severity)
	{
	case EKzValidationSeverity::Error:	 return FAppStyle::Get().GetBrush("Icons.ErrorWithColor");
	case EKzValidationSeverity::Warning: return FAppStyle::Get().GetBrush("Icons.WarningWithColor");
	case EKzValidationSeverity::Info:	 return FAppStyle::Get().GetBrush("Icons.InfoWithColor");
	}
	return FAppStyle::Get().GetBrush("Icons.Info");
}

FSlateColor SKzValidationPanel::GetSeverityColor(EKzValidationSeverity Severity) const
{
	// The "WithColor" brushes are pre-tinted, so we use UseForeground to keep them
	// pristine. Override here if you ever switch to monochrome icons.
	return FSlateColor::UseForeground();
}

FText SKzValidationPanel::GetSeverityLabel(EKzValidationSeverity Severity) const
{
	switch (Severity)
	{
	case EKzValidationSeverity::Error:	 return LOCTEXT("Error", "Errors");
	case EKzValidationSeverity::Warning: return LOCTEXT("Warning", "Warnings");
	case EKzValidationSeverity::Info:	 return LOCTEXT("Info", "Info");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
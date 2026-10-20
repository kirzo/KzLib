// Copyright 2026 kirzo

#include "KzLibEditorStyle.h"

FKzLibEditorStyle::FKzLibEditorStyle()
	: TKzEditorStyle_Base(FName("KzLibEditorStyle"))
{
	SetupPluginResources(TEXT("KzLib"));

	FSlateColor OutlineColor = FSlateColor(FLinearColor(0.1843f, 0.1843f, 0.1843f, 0.5f));
	FSlateColor OutlineColorSelected = FSlateColor(FLinearColor(0.1843f, 0.1843f, 0.1843f, 1.0f));

	// White line icon for lists, menus, the component tree and the Outliner; colour figure for the Content Browser and Place Actors
	const auto SetIcon = [this](const FName Class, const TCHAR* File)
	{
		Set(FName(*FString::Printf(TEXT("ClassIcon.%s"), *Class.ToString())), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + File, TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	};
	const auto SetThumbnail = [this](const FName Class, const TCHAR* File)
	{
		Set(FName(*FString::Printf(TEXT("ClassThumbnail.%s"), *Class.ToString())), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + File, TEXT(".svg")), FVector2D(64.0f, 64.0f)));
	};

	SetIcon("KzDatabaseAsset", TEXT("Database"));
	SetThumbnail("KzDatabaseAsset", TEXT("DatabaseColor"));
	SetIcon("KzDatabaseComponent", TEXT("Database"));
	SetIcon("KzShapeComponent", TEXT("Shape"));
	SetIcon("KzSplineActor", TEXT("Spline"));
	SetThumbnail("KzSplineActor", TEXT("SplineColor"));
	SetIcon("KzSplineFollowerComponent", TEXT("SplineFollower"));
	SetIcon("KzAreaNetwork", TEXT("SplineArea"));
	SetThumbnail("KzAreaNetwork", TEXT("SplineAreaColor"));
	SetIcon("KzSplineAreaComponent", TEXT("SplineArea"));
	SetIcon("KzActorGroup", TEXT("ActorGroup"));
	SetThumbnail("KzActorGroup", TEXT("ActorGroupColor"));

	Set("Kz.CardBorder", new FSlateRoundedBoxBrush(
		FStyleColors::Header, CoreStyleConstants::InputFocusRadius,
		OutlineColor, CoreStyleConstants::InputFocusThickness));

	Set("Kz.CardBorderSelected", new FSlateRoundedBoxBrush(
		FStyleColors::Select, CoreStyleConstants::InputFocusRadius,
		OutlineColorSelected, CoreStyleConstants::InputFocusThickness));

	Set("Kz.ListRowBorder", new FSlateRoundedBoxBrush(
		FStyleColors::Header, CoreStyleConstants::InputFocusRadius,
		OutlineColor, CoreStyleConstants::InputFocusThickness));

	Set("Kz.ListRowBorderSelected", new FSlateRoundedBoxBrush(
		FStyleColors::Select, CoreStyleConstants::InputFocusRadius,
		OutlineColorSelected, CoreStyleConstants::InputFocusThickness));

	Set("Kz.GroupBorder", new FSlateRoundedBoxBrush(
		FStyleColors::Recessed, CoreStyleConstants::InputFocusRadius));
}
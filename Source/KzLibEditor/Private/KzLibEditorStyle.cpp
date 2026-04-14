// Copyright 2026 kirzo

#include "KzLibEditorStyle.h"

FKzLibEditorStyle::FKzLibEditorStyle()
	: TKzEditorStyle_Base(FName("KzLibEditorStyle"))
{
	SetupPluginResources(TEXT("KzLib"));

	FSlateColor OutlineColor = FSlateColor(FLinearColor(0.1843f, 0.1843f, 0.1843f, 0.5f));
	FSlateColor OutlineColorSelected = FSlateColor(FLinearColor(0.1843f, 0.1843f, 0.1843f, 1.0f));

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
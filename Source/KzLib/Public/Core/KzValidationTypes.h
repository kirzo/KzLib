// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "KzValidationTypes.generated.h"

UENUM(BlueprintType)
enum class EKzValidationSeverity : uint8
{
	Info,
	Warning,
	Error,
};

/**
 * A single issue produced by a validator. Validators populate an array of these
 * during a validation pass; the editor UI presents them and (optionally) lets the
 * user jump to the affected element via ContextId or ContextIndex.
 */
USTRUCT(BlueprintType)
struct FKzValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	EKzValidationSeverity Severity = EKzValidationSeverity::Warning;

	/** Human-readable message describing the issue. */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FText Message;

	/** Optional GUID identifying the affected element (e.g. a dialogue line's LineId). */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FGuid ContextId;

	/**
	 * Optional array index of the affected element.
	 * Used as a fallback when ContextId is not applicable.
	 * INDEX_NONE means "no specific element".
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	int32 ContextIndex = INDEX_NONE;

	/**
	 * Identifier of the validator that produced the issue.
	 * Useful for grouping/filtering, and shown in the UI as a subtle tag.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Validation")
	FName ValidatorId;

	/**
	 * Optional inline-repair pair. When QuickFix is bound, the panel shows a small button labelled
	 * with QuickFixLabel; clicking it invokes QuickFix and triggers a panel refresh. Not reflected
	 * and not serialized — purely runtime editor data set by the producing validator.
	 */
	FText QuickFixLabel;
	TFunction<void()> QuickFix;

	/**
	 * Optional secondary navigation, invoked when the issue is activated after the editor has jumped
	 * to ContextId/ContextIndex. Lets a producer select a sub-element (e.g. a specific notify inside
	 * the line's timeline). Not reflected and not serialized -- runtime editor data.
	 */
	TFunction<void()> OnActivate;

	FKzValidationIssue() = default;

	FKzValidationIssue(EKzValidationSeverity InSeverity, const FText& InMessage, FName InValidatorId)
		: Severity(InSeverity), Message(InMessage), ContextIndex(INDEX_NONE), ValidatorId(InValidatorId)
	{
	}

	/** Convenience builder that sets a context GUID. */
	static FKzValidationIssue WithContextId(EKzValidationSeverity Severity, const FText& Message, FName ValidatorId, const FGuid& ContextId)
	{
		FKzValidationIssue Issue(Severity, Message, ValidatorId);
		Issue.ContextId = ContextId;
		return Issue;
	}

	/** Convenience builder that sets a context array index. */
	static FKzValidationIssue WithContextIndex(EKzValidationSeverity Severity, const FText& Message, FName ValidatorId, int32 Index)
	{
		FKzValidationIssue Issue(Severity, Message, ValidatorId);
		Issue.ContextIndex = Index;
		return Issue;
	}
};
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SNullWidget.h"

class IPropertyHandle;
class SWidget;
struct FSlateBrush;
class FAssetEditorToolkit;

/**
 * Extension point for customizing rows inside a SKzPropertyStack.
 *
 * Each consumer of FKzArrayAssetEditor (or any other host that uses SKzPropertyStack)
 * can subclass this and override only the methods that matter for that asset, providing
 * leading icons, trailing action buttons, custom display text, tooltips, etc.
 *
 * The default implementation returns null/empty for everything, so subclasses only
 * need to override what they want to change. The SKzPropertyStack will fall back to
 * its built-in behaviour for any null result.
 */
class KZLIBEDITOR_API FKzPropertyStackRowCustomizer : public TSharedFromThis<FKzPropertyStackRowCustomizer>
{
public:
	virtual ~FKzPropertyStackRowCustomizer() = default;

	// ---------------------------------------------------------------------------
	// Lifecycle
	// ---------------------------------------------------------------------------

	/** Called when the customizer is bound to an editor. */
	virtual void OnRegister(FAssetEditorToolkit* InHostEditor) {}

	/** Called when the customizer is unbound (editor closing). */
	virtual void OnUnregister() {}

	// ---------------------------------------------------------------------------
	// Per-row hooks
	// ---------------------------------------------------------------------------

	/**
	 * Widget shown to the left of the display text (between drag handle and label).
	 * Common uses: status icons, type icons, validity indicators.
	 * Return SNullWidget::NullWidget to keep the slot empty.
	 */
	virtual TSharedRef<SWidget> BuildLeadingWidget(TSharedPtr<IPropertyHandle> Handle)
	{
		return SNullWidget::NullWidget;
	}

	/**
	 * Widget shown to the right of the display text (between label and delete button).
	 * Common uses: action buttons (play, validate, jump-to), inline editors.
	 * Return SNullWidget::NullWidget to keep the slot empty.
	 */
	virtual TSharedRef<SWidget> BuildTrailingWidget(TSharedPtr<IPropertyHandle> Handle)
	{
		return SNullWidget::NullWidget;
	}

	/**
	 * Override the display text shown for the row.
	 * Return an empty FText to fall back to SKzPropertyStack's default
	 * resolution (TitleProperty meta -> object class name -> generic index). */
	virtual FText GetDisplayText(TSharedPtr<IPropertyHandle> Handle) const
	{
		return FText::GetEmpty();
	}

	/**
	 * Override the tooltip text shown when hovering the row.
	 * Return an empty FText to fall back to the property handle's own tooltip.
	 */
	virtual FText GetTooltipText(TSharedPtr<IPropertyHandle> Handle) const
	{
		return FText::GetEmpty();
	}

	/**
	 * Override the background brush used by the row. Return nullptr to fall back to
	 * the default Kz.CardBorder / Kz.CardBorderSelected brushes.
	 *
	 * Useful for tinting rows by status (e.g. a missing-audio dialogue line shown in
	 * red, a completed quest shown in green). */
	virtual const FSlateBrush* GetBackgroundBrush(TSharedPtr<IPropertyHandle> Handle, bool bIsSelected) const
	{
		return nullptr;
	}

	/**
	 * Build a custom "Add" widget that replaces the stack's default add button.
	 * Use this when adding an element requires more than just appending a default-
	 * constructed value (e.g. opening a picker scoped to the current asset).
	 *
	 * Receives the array handle so the implementation can append items itself.
	 * Return nullptr (default) to fall back to the stack's built-in add behavior.
	 */
	virtual TSharedPtr<SWidget> BuildAddWidget(TSharedPtr<class IPropertyHandleArray> /*ArrayHandle*/)
	{
		return nullptr;
	}

	// ---------------------------------------------------------------------------
	// Context resolution (used by validation jump-to and other navigation features)
	// ---------------------------------------------------------------------------

	/**
	 * Resolve a context GUID (e.g. a dialogue line's LineId) to the property handle
	 * representing it. Customizers that work with structs containing stable IDs
	 * override this to enable navigation features. Default: not implemented.
	 */
	virtual bool TryResolveContextId(const FGuid& ContextId, const TArray<TSharedPtr<IPropertyHandle>>& /*Handles*/, TSharedPtr<IPropertyHandle>& OutHandle) const
	{
		return false;
	}

	// ---------------------------------------------------------------------------
	// Multi-selection
	// ---------------------------------------------------------------------------

	/**
	 * Whether the property stack should allow selecting multiple rows at once.
	 * Customizers that don't make sense in multi (e.g. an audition widget that
	 * plays one sound at a time) can return false to force single-selection mode.
	 */
	virtual bool AllowsMultiSelect() const { return true; }

	// ---------------------------------------------------------------------------
	// Helpers for subclasses
	// ---------------------------------------------------------------------------

protected:
	/** Cast the row's value to a concrete struct type. Returns nullptr on failure. */
	template <typename T>
	static T* ResolveStruct(TSharedPtr<IPropertyHandle> Handle)
	{
		if (!Handle.IsValid()) { return nullptr; }
		void* Data = nullptr;
		if (Handle->GetValueData(Data) != FPropertyAccess::Success) { return nullptr; }
		return static_cast<T*>(Data);
	}

	/** Cast the row's value to a UObject pointer. Returns nullptr on failure. */
	template <typename T = UObject>
	static T* ResolveObject(TSharedPtr<IPropertyHandle> Handle)
	{
		if (!Handle.IsValid()) { return nullptr; }
		UObject* Obj = nullptr;
		if (Handle->GetValue(Obj) != FPropertyAccess::Success) { return nullptr; }
		return Cast<T>(Obj);
	}
};

/** Convenient alias used throughout the codebase. */
using FKzPropertyStackRowCustomizerPtr = TSharedPtr<FKzPropertyStackRowCustomizer>;
using FKzPropertyStackRowCustomizerRef = TSharedRef<FKzPropertyStackRowCustomizer>;
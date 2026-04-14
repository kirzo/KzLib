// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Factories.h"
#include "UnrealExporter.h"
#include "Exporters/Exporter.h"
#include "Misc/StringOutputDevice.h"
#include "HAL/PlatformApplicationMisc.h"
#include "ScopedTransaction.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"

// --- Custom Drag and Drop Operation Template ---
template<typename T>
class TKzObjectDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(TKzObjectDragDropOp, FDecoratedDragDropOp)

	T* ObjectToDrag = nullptr;

	static TSharedRef<TKzObjectDragDropOp<T>> New(T* InObject)
	{
		TSharedRef<TKzObjectDragDropOp<T>> Operation = MakeShared<TKzObjectDragDropOp<T>>();
		Operation->ObjectToDrag = InObject;

		if (InObject)
		{
			Operation->DefaultHoverText = InObject->GetClass()->GetDisplayNameText();
		}

		Operation->Construct();
		return Operation;
	}
};

// --- Text Object Factory for Pasting Template ---
template<typename T>
class TKzObjectTextFactory : public FCustomizableTextObjectFactory
{
public:
	TKzObjectTextFactory() : FCustomizableTextObjectFactory(GWarn) {}

	T* CreatedObject = nullptr;

	virtual bool CanCreateClass(UClass* ObjectClass, bool& bOmitSubObjs) const override
	{
		return ObjectClass->IsChildOf(T::StaticClass());
	}

	virtual void ProcessConstructedObject(UObject* InCreatedObject) override
	{
		CreatedObject = Cast<T>(InCreatedObject);
	}
};

// --- Class Filter ---
template<typename T>
class FKzClassViewerFilter : public IClassViewerFilter
{
public:
	TSet<const UClass*> DisallowedClasses;
	bool bAllowDuplicates = false;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		// Only accept children of T
		if (!InClass->IsChildOf(T::StaticClass())) return false;

		// Filter out abstract, deprecated, or temporary skeleton/reinstanced classes
		if (InClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) return false;
		if (InClass->GetName().StartsWith(TEXT("SKEL_")) || InClass->GetName().StartsWith(TEXT("REINST_"))) return false;

		// If duplicates are not allowed and the class is already in the target array, hide it
		if (!bAllowDuplicates && DisallowedClasses.Contains(InClass)) return false;

		return true;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return InUnloadedClassData->IsChildOf(T::StaticClass());
	}
};

// --- Clipboard Utilities ---
struct FKzClipboardUtils
{
	static void CopyObjectToClipboard(UObject* ObjectToCopy)
	{
		if (!ObjectToCopy) return;

		FStringOutputDevice Archive;
		const FExportObjectInnerContext Context;

		UExporter::ExportToOutputDevice(
			&Context,
			ObjectToCopy,
			nullptr,
			Archive,
			TEXT("copy"),
			0,
			PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited,
			false,
			ObjectToCopy->GetOuter()
		);

		FPlatformApplicationMisc::ClipboardCopy(*Archive);
	}

	template<typename TItemClass>
	static TItemClass* PasteObjectFromClipboard(UObject* Outer)
	{
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		if (TextToImport.IsEmpty() || !Outer) return nullptr;

		TKzObjectTextFactory<TItemClass> Factory;
		if (Factory.CanCreateObjectsFromText(TextToImport))
		{
			Factory.ProcessBuffer(Outer, RF_Transactional, TextToImport);
			return Factory.CreatedObject;
		}

		return nullptr;
	}
};
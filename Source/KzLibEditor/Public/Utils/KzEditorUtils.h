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

	/** Last object processed (kept for single-object call sites). */
	T* CreatedObject = nullptr;

	/** Every top-level object processed, in the order they appeared in the buffer. */
	TArray<T*> CreatedObjects;

	virtual bool CanCreateClass(UClass* ObjectClass, bool& bOmitSubObjs) const override
	{
		return ObjectClass->IsChildOf(T::StaticClass());
	}

	virtual void ProcessConstructedObject(UObject* InCreatedObject) override
	{
		if (T* Cast = ::Cast<T>(InCreatedObject))
		{
			CreatedObject = Cast;
			CreatedObjects.Add(Cast);
		}
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
		CopyObjectsToClipboard(MakeArrayView(&ObjectToCopy, 1));
	}

	/** Exports several objects into a single T3D blob and writes it to the clipboard. */
	static void CopyObjectsToClipboard(TArrayView<UObject* const> ObjectsToCopy)
	{
		if (ObjectsToCopy.IsEmpty()) return;

		FStringOutputDevice Archive;
		const FExportObjectInnerContext Context;

		for (UObject* Obj : ObjectsToCopy)
		{
			if (!Obj) continue;

			UExporter::ExportToOutputDevice(
				&Context,
				Obj,
				nullptr,
				Archive,
				TEXT("copy"),
				0,
				PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited,
				false,
				Obj->GetOuter()
			);
		}

		FPlatformApplicationMisc::ClipboardCopy(*Archive);
	}

	template<typename TItemClass = UObject>
	static TItemClass* PasteObjectFromClipboard(UObject* Outer)
	{
		TArray<TItemClass*> All = PasteObjectsFromClipboard<TItemClass>(Outer);
		return All.IsEmpty() ? nullptr : All[0];
	}

	/**
	 * Imports every top-level object stored in the clipboard whose class derives from TItemClass.
	 * Returned objects keep the order in which they were exported.
	 */
	template<typename TItemClass = UObject>
	static TArray<TItemClass*> PasteObjectsFromClipboard(UObject* Outer)
	{
		TArray<TItemClass*> Result;

		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		if (TextToImport.IsEmpty() || !Outer) return Result;

		TKzObjectTextFactory<TItemClass> Factory;
		if (Factory.CanCreateObjectsFromText(TextToImport))
		{
			Factory.ProcessBuffer(Outer, RF_Transactional, TextToImport);
			Result = MoveTemp(Factory.CreatedObjects);
		}

		return Result;
	}
};

// --- PropertyHandle Utilities ---
struct FKzPropertyHandleUtils
{
	/** Returns true if PropertyHandle or any of its ancestor handles have the given metadata. */
	static bool HasMetaDataInHierarchy(TSharedPtr<IPropertyHandle> PropertyHandle, FName MetaKey)
	{
		for (TSharedPtr<IPropertyHandle> Current = PropertyHandle; Current.IsValid(); Current = Current->GetParentHandle())
		{
			if (Current->HasMetaData(MetaKey))
			{
				return true;
			}
		}
		return false;
	}

	/** Returns the metadata value of PropertyHandle or the nearest ancestor that has it. Empty string if not found. */
	static const FString& GetMetaDataInHierarchy(TSharedPtr<IPropertyHandle> PropertyHandle, FName MetaKey)
	{
		for (TSharedPtr<IPropertyHandle> Current = PropertyHandle; Current.IsValid(); Current = Current->GetParentHandle())
		{
			if (Current->HasMetaData(MetaKey))
			{
				return Current->GetMetaData(MetaKey);
			}
		}
		static const FString Empty;
		return Empty;
	}
};
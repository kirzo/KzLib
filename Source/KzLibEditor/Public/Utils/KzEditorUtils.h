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
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"

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
			// As the engine's edit-inline creation: a template subobject must be Public to be the archetype of instances in other packages.
			Factory.ProcessBuffer(Outer, RF_Transactional | Outer->GetMaskedFlags(RF_PropagateToSubObjects), TextToImport);
			Result = MoveTemp(Factory.CreatedObjects);
		}

		return Result;
	}
};

// --- PropertyHandle Utilities ---
struct FKzPropertyHandleUtils
{
	/**
	 * Adds every child of StructHandle to the builder, honoring ShowOnlyInnerProperties on
	 * struct children: their inner properties are added in place of the struct's own row
	 * (recursively), the way default detail layouts flatten and plain AddProperty loops don't.
	 * SkipChildren names top-level children the caller renders manually.
	 */
	static void AddChildrenHonoringInnerProperties(IDetailChildrenBuilder& StructBuilder, TSharedRef<IPropertyHandle> StructHandle, const TSet<FName>& SkipChildren = TSet<FName>())
	{
		uint32 NumChildren = 0;
		StructHandle->GetNumChildren(NumChildren);
		for (uint32 i = 0; i < NumChildren; ++i)
		{
			TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(i);
			if (!Child.IsValid()) { continue; }

			const FProperty* Property = Child->GetProperty();
			if (Property && SkipChildren.Contains(Property->GetFName())) { continue; }

			if (Property && Property->IsA<FStructProperty>() && Property->HasMetaData(TEXT("ShowOnlyInnerProperties")))
			{
				AddChildrenHonoringInnerProperties(StructBuilder, Child.ToSharedRef());
				continue;
			}

			StructBuilder.AddProperty(Child.ToSharedRef());
		}
	}

	/**
	 * Like AddChildrenHonoringInnerProperties, but children whose Category has three or more
	 * segments render inside a collapsible group named after the last segment ("A|B|Audio"
	 * puts the child in an "Audio" group). One- and two-segment categories stay flat, in
	 * declaration order; each group appears at the position of its first member and collects
	 * later members regardless of interleaving. OutGroups (optional) receives the created
	 * groups by name so callers can append custom rows into them. CustomizeChild (optional)
	 * may claim a child and add its row itself (return true); claimed rows land at the root,
	 * at their declaration position.
	 */
	static void AddChildrenGroupedByCategory(IDetailChildrenBuilder& StructBuilder, TSharedRef<IPropertyHandle> StructHandle, const TSet<FName>& SkipChildren = TSet<FName>(), TMap<FString, IDetailGroup*>* OutGroups = nullptr, const TFunction<bool(IDetailChildrenBuilder&, TSharedRef<IPropertyHandle>)>& CustomizeChild = nullptr)
	{
		TMap<FString, IDetailGroup*> LocalGroups;
		AddChildrenGroupedByCategoryInner(StructBuilder, StructHandle, SkipChildren, OutGroups ? *OutGroups : LocalGroups, CustomizeChild);
	}

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

private:
	/** Recursion body of AddChildrenGroupedByCategory; Groups persists across the ShowOnlyInnerProperties flattening. */
	static void AddChildrenGroupedByCategoryInner(IDetailChildrenBuilder& StructBuilder, TSharedRef<IPropertyHandle> StructHandle, const TSet<FName>& SkipChildren, TMap<FString, IDetailGroup*>& Groups, const TFunction<bool(IDetailChildrenBuilder&, TSharedRef<IPropertyHandle>)>& CustomizeChild = nullptr)
	{
		uint32 NumChildren = 0;
		StructHandle->GetNumChildren(NumChildren);
		for (uint32 i = 0; i < NumChildren; ++i)
		{
			TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(i);
			if (!Child.IsValid()) { continue; }

			const FProperty* Property = Child->GetProperty();
			if (Property && SkipChildren.Contains(Property->GetFName())) { continue; }

			if (CustomizeChild && CustomizeChild(StructBuilder, Child.ToSharedRef())) { continue; }

			if (Property && Property->IsA<FStructProperty>() && Property->HasMetaData(TEXT("ShowOnlyInnerProperties")))
			{
				AddChildrenGroupedByCategoryInner(StructBuilder, Child.ToSharedRef(), TSet<FName>(), Groups, CustomizeChild);
				continue;
			}

			TArray<FString> Segments;
			if (Property)
			{
				Property->GetMetaData(TEXT("Category")).ParseIntoArray(Segments, TEXT("|"));
			}
			if (Segments.Num() >= 3)
			{
				const FString& GroupName = Segments.Last();
				IDetailGroup*& Group = Groups.FindOrAdd(GroupName);
				if (!Group)
				{
					Group = &StructBuilder.AddGroup(FName(*GroupName), FText::FromString(GroupName));
				}
				Group->AddPropertyRow(Child.ToSharedRef());
			}
			else
			{
				StructBuilder.AddProperty(Child.ToSharedRef());
			}
		}
	}
};
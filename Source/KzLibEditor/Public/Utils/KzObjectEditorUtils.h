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
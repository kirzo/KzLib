// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/KzVariant.h"
#include "Core/KzTypes.h"
#include "KzVariantLibrary.generated.h"

/** Blueprint API for FKzVariant. */
UCLASS()
class KZLIB_API UKzVariantLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Builds a variant from any supported value. The CustomThunk reads the value's type from the calling FProperty. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", CustomThunk, meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true"))
	static FKzVariant MakeKzVariant(const int32& Value);

	/** Writes a value into an existing variant, replacing whatever it held. */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Variant", CustomThunk, meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true"))
	static UPARAM(ref) FKzVariant& SetKzVariant(UPARAM(ref) FKzVariant& Variant, const int32& Value);

	/** Reads a variant into a typed output. Equivalent to BreakKzVariant; offered as a separate node for clarity. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", CustomThunk, meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true"))
	static bool GetKzVariant(const FKzVariant& Variant, int32& Value);

	UFUNCTION(BlueprintCallable, Category = "KzLib|Variant", CustomThunk, meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true", ExpandEnumAsExecs = "ExecResult"))
	static void BreakKzVariant(EKzValidity& ExecResult, const FKzVariant& Variant, int32& Value);

	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", meta = (DisplayName = "Is Valid"))
	static bool IsValidKzVariant(const FKzVariant& Variant) { return Variant.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", meta = (DisplayName = "Get Type"))
	static EPropertyBagPropertyType GetKzVariantType(const FKzVariant& Variant) { return Variant.GetType(); }

	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", meta = (DisplayName = "Get Type Object"))
	static const UObject* GetKzVariantTypeObject(const FKzVariant& Variant) { return Variant.GetTypeObject(); }

	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", meta = (DisplayName = "Equal (KzVariant)", CompactNodeTitle = "==", Keywords = "== equal"))
	static bool EqualEqual_KzVariant(const FKzVariant& A, const FKzVariant& B) { return A == B; }

	UFUNCTION(BlueprintPure, Category = "KzLib|Variant", meta = (DisplayName = "Not Equal (KzVariant)", CompactNodeTitle = "!=", Keywords = "!= not equal"))
	static bool NotEqual_KzVariant(const FKzVariant& A, const FKzVariant& B) { return A != B; }

private:
	DECLARE_FUNCTION(execMakeKzVariant);
	DECLARE_FUNCTION(execSetKzVariant);
	DECLARE_FUNCTION(execGetKzVariant);
	DECLARE_FUNCTION(execBreakKzVariant);
};
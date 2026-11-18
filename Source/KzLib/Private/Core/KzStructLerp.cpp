// Copyright 2026 kirzo

#include "Core/KzStructLerp.h"
#include "UObject/ObjectKey.h"
#include "UObject/UnrealType.h"

namespace Kz::StructLerp
{
	enum class EKind : uint8
	{
		Float,
		Double,
		Int,
		Color,
		Rotator,
		Quat,
		Step
	};

	struct FOp
	{
		EKind Kind = EKind::Step;
		int32 Offset = 0;

		/** For Step: copies the whole value. */
		const FProperty* Property = nullptr;

		/** The bOverride_ flag that gates this value, if any. */
		const FBoolProperty* Flag = nullptr;
		int32 FlagOffset = 0;
	};

	static bool IsOverrideFlag(const UStruct* Struct, const FProperty* Property)
	{
		FString Name = Property->GetName();
		return Property->IsA<FBoolProperty>() && Name.RemoveFromStart(TEXT("bOverride_")) && FindFProperty<FProperty>(Struct, *Name);
	}

	static EKind GetKind(const FProperty* Property, const UScriptStruct*& OutInner)
	{
		OutInner = nullptr;
		if (Property->IsA<FFloatProperty>()) return EKind::Float;
		if (Property->IsA<FDoubleProperty>()) return EKind::Double;
		if (Property->IsA<FIntProperty>()) return EKind::Int;

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == TBaseStructure<FColor>::Get()) return EKind::Color;
			if (StructProperty->Struct == TBaseStructure<FRotator>::Get()) return EKind::Rotator;
			if (StructProperty->Struct == TBaseStructure<FQuat>::Get()) return EKind::Quat;
			OutInner = StructProperty->Struct;
		}

		return EKind::Step;
	}

	/** Flattens Struct into ops, at BaseOffset from the outermost struct. Members of a nested struct inherit its flag. */
	static void Build(const UStruct* Struct, int32 BaseOffset, const FBoolProperty* Flag, int32 FlagOffset, TArray<FOp>& Ops)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Property = *It;
			if (IsOverrideFlag(Struct, Property)) continue;

			const FBoolProperty* OwnFlag = FindFProperty<FBoolProperty>(Struct, *(FString(TEXT("bOverride_")) + Property->GetName()));
			const FBoolProperty* PropertyFlag = OwnFlag ? OwnFlag : Flag;
			const int32 PropertyFlagOffset = OwnFlag ? BaseOffset + OwnFlag->GetOffset_ForInternal() : FlagOffset;
			const int32 Offset = BaseOffset + Property->GetOffset_ForInternal();
			const FOp WholeValue{ EKind::Step, Offset, Property, PropertyFlag, PropertyFlagOffset };

			const UScriptStruct* Inner = nullptr;
			const EKind Kind = GetKind(Property, Inner);

			if (Kind == EKind::Step && !Inner)
			{
				Ops.Add(WholeValue);
				continue;
			}

			TArray<FOp> ElementOps;
			for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
			{
				const int32 ElementOffset = Offset + Index * Property->GetElementSize();
				if (Inner)
				{
					Build(Inner, ElementOffset, PropertyFlag, PropertyFlagOffset, ElementOps);
				}
				else
				{
					ElementOps.Add({ Kind, ElementOffset, nullptr, PropertyFlag, PropertyFlagOffset });
				}
			}

			// A struct with nothing to interpolate is copied whole, with whatever it keeps outside its UPROPERTYs.
			if (ElementOps.ContainsByPredicate([](const FOp& Op) { return Op.Kind != EKind::Step; }))
			{
				Ops.Append(ElementOps);
			}
			else
			{
				Ops.Add(WholeValue);
			}
		}
	}

	static const TArray<FOp>& GetPlan(const UScriptStruct* Struct)
	{
		check(IsInGameThread());

		// Keyed by object key, so a struct recompiled into the same address gets a new plan.
		static TMap<TObjectKey<UScriptStruct>, TArray<FOp>> Plans;

		if (const TArray<FOp>* Plan = Plans.Find(Struct))
		{
			return *Plan;
		}

		TArray<FOp>& Plan = Plans.Add(Struct);
		Build(Struct, 0, nullptr, 0, Plan);
		return Plan;
	}

	template <typename T>
	static T& At(void* Data, int32 Offset)
	{
		return *reinterpret_cast<T*>(static_cast<uint8*>(Data) + Offset);
	}

	template <typename T>
	static const T& At(const void* Data, int32 Offset)
	{
		return *reinterpret_cast<const T*>(static_cast<const uint8*>(Data) + Offset);
	}
}

void Kz::LerpStruct(const UScriptStruct* Struct, void* Out, const void* B, float Alpha)
{
	using namespace StructLerp;

	if (!Struct || !Out || !B || Alpha <= 0.0f) return;
	Alpha = FMath::Min(Alpha, 1.0f);

	for (const FOp& Op : GetPlan(Struct))
	{
		if (Op.Flag)
		{
			if (!Op.Flag->GetPropertyValue(static_cast<const uint8*>(B) + Op.FlagOffset)) continue;
			Op.Flag->SetPropertyValue(static_cast<uint8*>(Out) + Op.FlagOffset, true);
		}

		switch (Op.Kind)
		{
		case EKind::Float:
		{
			float& Value = At<float>(Out, Op.Offset);
			Value = FMath::Lerp(Value, At<float>(B, Op.Offset), Alpha);
			break;
		}
		case EKind::Double:
		{
			double& Value = At<double>(Out, Op.Offset);
			Value = FMath::Lerp(Value, At<double>(B, Op.Offset), static_cast<double>(Alpha));
			break;
		}
		case EKind::Int:
		{
			int32& Value = At<int32>(Out, Op.Offset);
			Value = FMath::RoundToInt(FMath::Lerp(static_cast<double>(Value), static_cast<double>(At<int32>(B, Op.Offset)), static_cast<double>(Alpha)));
			break;
		}
		case EKind::Color:
		{
			FColor& Value = At<FColor>(Out, Op.Offset);
			const FLinearColor From(Value);
			const FLinearColor To(At<FColor>(B, Op.Offset));
			Value = (From + (To - From) * Alpha).ToFColorSRGB();
			break;
		}
		case EKind::Rotator:
		{
			FRotator& Value = At<FRotator>(Out, Op.Offset);
			Value = FQuat::Slerp(Value.Quaternion(), At<FRotator>(B, Op.Offset).Quaternion(), Alpha).Rotator();
			break;
		}
		case EKind::Quat:
		{
			FQuat& Value = At<FQuat>(Out, Op.Offset);
			Value = FQuat::Slerp(Value, At<FQuat>(B, Op.Offset), Alpha);
			break;
		}
		case EKind::Step:
		{
			if (Alpha >= 0.5f)
			{
				Op.Property->CopyCompleteValue(static_cast<uint8*>(Out) + Op.Offset, static_cast<const uint8*>(B) + Op.Offset);
			}
			break;
		}
		}
	}
}

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/KzStructLerpTestTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKzStructLerpTest, "KzLib.Core.LerpStruct", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKzStructLerpTest::RunTest(const FString& Parameters)
{
	const UScriptStruct* Struct = FKzStructLerpTestStruct::StaticStruct();

	FKzStructLerpTestStruct A;
	A.Float = 0.0f;
	A.Int = 0;
	A.Linear = FLinearColor::Black;
	A.Color = FColor::Black;
	A.Rotator = FRotator(0.0f, 350.0f, 0.0f);
	A.bBool = false;
	A.Name = TEXT("A");
	A.Inner.Value = 0.0;
	A.Gated = 0.0f;
	A.Ungated = 0.0f;

	FKzStructLerpTestStruct B;
	B.Float = 10.0f;
	B.Int = 10;
	B.Linear = FLinearColor::White;
	B.Color = FColor::White;
	B.Rotator = FRotator(0.0f, 10.0f, 0.0f);
	B.bBool = true;
	B.Name = TEXT("B");
	B.Inner.Value = 4.0;
	B.Gated = 10.0f;
	B.Ungated = 10.0f;
	B.bOverride_Gated = false;
	B.bOverride_Ungated = true;

	FKzStructLerpTestStruct Out = A;
	Kz::LerpStruct(Struct, &Out, &B, 0.25f);

	TestEqual(TEXT("Float"), Out.Float, 2.5f);
	TestEqual(TEXT("Int rounds"), Out.Int, 3);
	TestTrue(TEXT("Linear color"), Out.Linear.Equals(FLinearColor(0.25f, 0.25f, 0.25f, 1.0f)));
	TestEqual(TEXT("Nested struct"), Out.Inner.Value, 1.0);
	TestFalse(TEXT("Bool waits for 0.5"), Out.bBool);
	TestTrue(TEXT("Name waits for 0.5"), Out.Name == FName(TEXT("A")));
	TestEqual(TEXT("Not overridden in B"), Out.Gated, 0.0f);
	TestFalse(TEXT("Its flag stays off"), Out.bOverride_Gated);
	TestEqual(TEXT("Overridden in B"), Out.Ungated, 2.5f);
	TestTrue(TEXT("Its flag turns on"), Out.bOverride_Ungated);

	Out = A;
	Kz::LerpStruct(Struct, &Out, &B, 0.5f);

	TestTrue(TEXT("Rotator takes the short way"), Out.Rotator.Equals(FRotator::ZeroRotator, 0.01f));
	TestEqual(TEXT("FColor blends in linear space"), Out.Color, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f).ToFColorSRGB());
	TestTrue(TEXT("Bool switches at 0.5"), Out.bBool);
	TestTrue(TEXT("Name switches at 0.5"), Out.Name == FName(TEXT("B")));

	Out = A;
	Kz::LerpStruct(Struct, &Out, &B, 0.0f);
	TestEqual(TEXT("Alpha 0 leaves it alone"), Out.Float, 0.0f);

	return true;
}

#endif
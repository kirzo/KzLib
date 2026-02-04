// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/EnableIf.h"
#include "Concepts/BaseStructureProvider.h"

namespace KzPropertyBag
{
	namespace Private
	{
		/** Check if a type is a UStruct/ScriptStruct compatible type */
		template <typename ValueType>
		constexpr bool TIsStructure = TModels_V<CBaseStructureProvider, std::decay_t<ValueType>>;

		/** Helper to extract inner type from TArray */
		template <typename T> struct TUnwrapArray { using Type = T; static constexpr bool IsArray = false; };
		template <typename T> struct TUnwrapArray<TArray<T>> { using Type = T; static constexpr bool IsArray = true; };
	}

	/**
	 * Main template struct to handle PropertyBag type mapping.
	 * Specializations provide Type Enum, ObjectType getter, and Get/Set methods.
	 */
	template <typename TPropertyType, typename Enable = void>
	struct TPropertyBagType;

	// --- Specialization: Bool ---
	template<>
	struct TPropertyBagType<bool>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Bool;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<bool, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueBool(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, bool bValue)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueBool(Name, bValue);
		}
	};

	// --- Specialization: Int32 ---
	template<>
	struct TPropertyBagType<int32>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Int32;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<int32, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueInt32(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, int32 Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueInt32(Name, Value);
		}
	};

	// --- Specialization: Int64 ---
	template<>
	struct TPropertyBagType<int64>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Int64;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<int64, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueInt64(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, int64 Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueInt64(Name, Value);
		}
	};

	// --- Specialization: Byte (uint8) ---
	template<>
	struct TPropertyBagType<uint8>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Byte;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<uint8, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueByte(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, uint8 Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueByte(Name, Value);
		}
	};

	// --- Specialization: Float ---
	template<>
	struct TPropertyBagType<float>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Float;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<float, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueFloat(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, float Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueFloat(Name, Value);
		}
	};

	// --- Specialization: Double ---
	template<>
	struct TPropertyBagType<double>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Double;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<double, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueDouble(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, double Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueDouble(Name, Value);
		}
	};

	// --- Specialization: FName ---
	template<>
	struct TPropertyBagType<FName>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Name;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<FName, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueName(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const FName& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueName(Name, Value);
		}
	};

	// --- Specialization: FString ---
	template<>
	struct TPropertyBagType<FString>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::String;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<FString, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueString(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const FString& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueString(Name, Value);
		}
	};

	// --- Specialization: FText ---
	template<>
	struct TPropertyBagType<FText>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Text;
		static const UObject* GetObjectType() { return nullptr; }

		static TValueOrError<FText, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueText(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const FText& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueText(Name, Value);
		}
	};

	// --- Specialization: Enums ---
	template <typename TPropertyType>
	struct TPropertyBagType<TPropertyType, typename TEnableIf<TIsEnum<TPropertyType>::Value>::Type>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Enum;
		static const UObject* GetObjectType() { return StaticEnum<TPropertyType>(); }

		static TValueOrError<TPropertyType, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueEnum<TPropertyType>(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const TPropertyType& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueEnum(Name, Value);
		}
	};

	// --- Specialization: Structs ---
	template <typename TPropertyType>
	struct TPropertyBagType<TPropertyType, typename TEnableIf<Private::TIsStructure<TPropertyType>>::Type>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Struct;

		static const UObject* GetObjectType()
		{
			return TBaseStructure<TPropertyType>::Get();
		}

		static TValueOrError<TPropertyType, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			TValueOrError<TPropertyType*, EPropertyBagResult> Result = Bag.GetValueStruct<TPropertyType>(Name);
			if (Result.HasError())
			{
				return MakeError(Result.GetError());
			}
			return MakeValue(*Result.GetValue());
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const TPropertyType& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueStruct(Name, Value);
		}
	};

	// --- Specialization: UObjects ---
	template <typename TPropertyType>
	struct TPropertyBagType<TPropertyType*, typename TEnableIf<TIsDerivedFrom<TPropertyType, UObject>::Value>::Type>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::Object;
		static const UObject* GetObjectType() { return TPropertyType::StaticClass(); }

		static TValueOrError<TPropertyType*, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			return Bag.GetValueObject<TPropertyType>(Name);
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, TPropertyType* Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueObject(Name, Value);
		}
	};

	// --- Specialization: Soft Object Pointers ---
	template <typename T>
	struct TPropertyBagType<TSoftObjectPtr<T>>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::SoftObject;
		static const UObject* GetObjectType() { return T::StaticClass(); }

		static TValueOrError<TSoftObjectPtr<T>, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			TValueOrError<FSoftObjectPath, EPropertyBagResult> Result = Bag.GetValueSoftPath(Name);
			if (Result.HasError())
			{
				return MakeError(Result.GetError());
			}
			return MakeValue(TSoftObjectPtr<T>(Result.GetValue()));
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const TSoftObjectPtr<T>& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueSoftPath(Name, Value.ToSoftObjectPath());
		}
	};

	// --- Specialization: Soft Class Pointers ---
	template <typename T>
	struct TPropertyBagType<TSoftClassPtr<T>>
	{
		static constexpr EPropertyBagPropertyType Type = EPropertyBagPropertyType::SoftClass;
		static const UObject* GetObjectType() { return T::StaticClass(); }

		static TValueOrError<TSoftClassPtr<T>, EPropertyBagResult> GetValue(const FInstancedPropertyBag& Bag, const FName& Name)
		{
			TValueOrError<FSoftObjectPath, EPropertyBagResult> Result = Bag.GetValueSoftPath(Name);
			if (Result.HasError())
			{
				return MakeError(Result.GetError());
			}
			return MakeValue(TSoftClassPtr<T>(Result.GetValue()));
		}

		static void SetValue(FInstancedPropertyBag& Bag, const FName& Name, const TSoftClassPtr<T>& Value)
		{
			Bag.AddProperty(Name, Type, GetObjectType());
			Bag.SetValueSoftPath(Name, Value.ToSoftObjectPath());
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Public API
	// ------------------------------------------------------------------------------------------------

	/**
	 * Generic Setter. Automatically infers type and adds the property to the bag if missing.
	 * Usage: KzPropertyBag::Set(MyBag, "Health", 100.0f);
	 */
	template <typename T>
	FORCEINLINE void Set(FInstancedPropertyBag& Bag, const FName& Name, const T& Value)
	{
		TPropertyBagType<T>::SetValue(Bag, Name, Value);
	}

	/**
	 * Generic Getter.
	 * Usage: float Health = KzPropertyBag::Get<float>(MyBag, "Health").GetValue();
	 */
	template <typename T>
	FORCEINLINE TValueOrError<T, EPropertyBagResult> Get(const FInstancedPropertyBag& Bag, const FName& Name)
	{
		return TPropertyBagType<T>::GetValue(Bag, Name);
	}

	/**
	 * Adds a property definition to the bag without setting a value.
	 * Usage: KzPropertyBag::Add<float>(MyBag, "Health");
	 */
	template <typename T>
	FORCEINLINE void Add(FInstancedPropertyBag& Bag, const FName& Name)
	{
		Bag.AddProperty(Name, TPropertyBagType<T>::Type, TPropertyBagType<T>::GetObjectType());
	}

} // End namespace KzPropertyBag
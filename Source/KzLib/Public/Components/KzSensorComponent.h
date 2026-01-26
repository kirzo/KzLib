// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Components/KzShapeComponent.h"
#include "KzSensorComponent.generated.h"

class UKzRegistrySubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzSensorObjectEvent, UObject*, DetectedObject);

/**
 * A sensor component that detects objects using KzLib's GJK intersection.
 * Optimized to maintain the link between the Physical Shape and the Logical Object
 * to avoid ambiguous resolution steps.
 */
UCLASS(ClassGroup = "KzLib", meta = (BlueprintSpawnableComponent))
class KZLIB_API UKzSensorComponent : public UKzShapeComponent
{
	GENERATED_BODY()

public:
	UKzSensorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Configuration ---

	/** The class type to look for in the KzRegistrySubsystem.*/
	UPROPERTY(EditAnywhere, Category = "Kz Sensor")
	TSubclassOf<UObject> AutoRegisterCategory;

	/** How often to perform the geometric scan (in seconds). */
	UPROPERTY(EditAnywhere, Category = "Kz Sensor", meta = (ClampMin = 0, Units = "Seconds"))
	float ScanInterval = 0.1f;

	// --- Events ---

	/** Fired when a resolved logical object enters the sensor. */
	UPROPERTY(BlueprintAssignable, Category = "Kz Sensor")
	FKzSensorObjectEvent OnObjectBeginOverlap;

	/** Fired when a resolved logical object exits the sensor. */
	UPROPERTY(BlueprintAssignable, Category = "Kz Sensor")
	FKzSensorObjectEvent OnObjectEndOverlap;

	// --- Public API ---

	/** Returns the list of currently overlapped logical objects cast to T. */
	template <typename T>
	TArray<T*> GetOverlappingObjects() const
	{
		TArray<T*> Result;
		Result.Reserve(CachedLogicObjects.Num());

		for (const TWeakObjectPtr<UObject>& WeakPtr : CachedLogicObjects)
		{
			if (UObject* Obj = WeakPtr.Get())
			{
				if (T* Casted = Cast<T>(Obj))
				{
					Result.Add(Casted);
				}
			}
		}
		return Result;
	}

private:
	void PerformScan();

	/** The cache of resolved logical objects. This is now the source of truth for events. */
	TArray<TWeakObjectPtr<UObject>> CachedLogicObjects;

	float TimeSinceLastScan = 0.0f;
};
// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzSystemLibrary.generated.h"

struct FKzHitResult;
struct FKzTransformSource;

// Whether to inline functions at all
#define KZ_KISMET_SYSTEM_INLINE_ENABLED	(!UE_BUILD_DEBUG)

/** General-purpose Blueprint utilities. */
UCLASS(meta = (BlueprintThreadSafe, ScriptName = "KzSystemLibrary"))
class KZLIB_API UKzSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	/** Extracts data from a HitResult.
	 * @param Hit			The source HitResult.
	 * @param bBlockingHit	True if there was a blocking hit, false otherwise.
	 * @param bInitialOverlap True if the hit started in an initial overlap. In this case some other values should be interpreted differently. Time will be 0, ImpactPoint will equal Location, and normals will be equal and indicate a depenetration vector.
	 * @param Time			'Time' of impact along trace direction ranging from [0.0 to 1.0) if there is a hit, indicating time between start and end. Equals 1.0 if there is no hit.
	 * @param Distance		The distance from the TraceStart to the Location in world space. This value is 0 if there was an initial overlap (trace started inside another colliding object).
	 * @param ImpactPoint		Location of the hit in world space. If this was a swept shape test, this is the location where we can place the shape in the world where it will not penetrate.
	 * @param Normal		Normal of the hit in world space, for the object that was swept (e.g. for a sphere trace this points towards the sphere's center). Equal to ImpactNormal for line tests.
	 */
	UFUNCTION(BlueprintPure, Category = Collision, meta = (NativeBreakFunc, AdvancedDisplay = "3"))
	static void BreakHitResult(const FKzHitResult& Hit, bool& bBlockingHit, bool& bInitialOverlap, float& Time, float& Distance, FVector& Location, FVector& Normal, FVector& TraceStart, FVector& TraceEnd);

public:

	// === FKzTransformSource ===

	/** Returns true if the given transform source is valid. */
	UFUNCTION(BlueprintPure, Category = "Kz|System", meta = (Keywords = "transform source"))
	static bool IsValid(const FKzTransformSource& Source);

	/** Converts an KzTransformSource to a Vector */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To Vector (KzTransformSource)", CompactNodeTitle = "->", ScriptMethod = "Vector", Keywords = "cast convert transform source", BlueprintAutocast))
	static FVector Conv_KzTransformSourceToVector(const FKzTransformSource& Source);

	/** Converts an KzTransformSource to a Rotator */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To Rotator (KzTransformSource)", CompactNodeTitle = "->", ScriptMethod = "Rotator", Keywords = "cast convert transform source", BlueprintAutocast))
	static FRotator Conv_KzTransformSourceToRotator(const FKzTransformSource& Source);

	/** Converts an KzTransformSource to a Transform */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To Transform (KzTransformSource)", CompactNodeTitle = "->", ScriptMethod = "Transform", Keywords = "cast convert transform source", BlueprintAutocast))
	static FTransform Conv_KzTransformSourceToTransform(const FKzTransformSource& Source);

	/** Converts an Vector to a KzTransformSource */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To KzTransformSource (Vector)", CompactNodeTitle = "->", ScriptMethod = "KzTransformSource", Keywords = "cast convert transform source", BlueprintAutocast))
	static FKzTransformSource Conv_VectorToKzTransformSource(const FVector& Vector);

	/** Converts an Rotator to a KzTransformSource */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To KzTransformSource (Rotator)", CompactNodeTitle = "->", ScriptMethod = "KzTransformSource", Keywords = "cast convert transform source", BlueprintAutocast))
	static FKzTransformSource Conv_RotatorToKzTransformSource(const FRotator& Rotator);

	/** Converts an Transform to a KzTransformSource */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To KzTransformSource (Transform)", CompactNodeTitle = "->", ScriptMethod = "KzTransformSource", Keywords = "cast convert transform source", BlueprintAutocast))
	static FKzTransformSource Conv_TransformToKzTransformSource(const FTransform& Source);

	/** Converts an Actor to a KzTransformSource, using its root component's world transform. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To KzTransformSource (Actor)", CompactNodeTitle = "->", ScriptMethod = "KzTransformSource", Keywords = "cast convert transform source", BlueprintAutocast))
	static FKzTransformSource Conv_ActorToKzTransformSource(const AActor* Actor);

	/** Converts a SceneComponent to a KzTransformSource. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Conversions", meta = (DisplayName = "To KzTransformSource (SceneComponent)", CompactNodeTitle = "->", ScriptMethod = "KzTransformSource", Keywords = "cast convert transform source", BlueprintAutocast))
	static FKzTransformSource Conv_SceneComponentToKzTransformSource(const USceneComponent* Component, const FName SocketName = NAME_None);

	// === Random ===

	/** Returns a Gaussian random float N(0,1) using global RNG. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Random")
	static float GaussianFloat();

	/** Returns a Gaussian random float N(0,1) using the provided RandomStream. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Random")
	static float GaussianFloatFromStream(UPARAM(ref) FRandomStream& Stream);

	/** Returns a Gaussian random vector N(0,1) using global RNG. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Random")
	static FVector GaussianVector();

	/** Returns a Gaussian random vector N(0,1) using the provided RandomStream. */
	UFUNCTION(BlueprintPure, Category = "KzLib|Random")
	static FVector GaussianVectorFromStream(UPARAM(ref) FRandomStream& Stream);
};

// Inline implementations
#if KZ_KISMET_SYSTEM_INLINE_ENABLED
#include "KzSystemLibrary.inl"
#endif
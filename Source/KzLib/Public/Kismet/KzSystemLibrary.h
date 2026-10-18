// Copyright 2025 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "KzSystemLibrary.generated.h"

struct FKzHitResult;
struct FKzTransformSource;
struct FKzComponentSocketReference;

class AController;

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

	// === FKzComponentSocketReference ===

	/**
	 * Resolves the reference to finding the actual Scene Component.
	 * @param Reference The socket reference struct.
	 * @param Context The object initiating the query.
	 * @return The resolved component, or nullptr if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context"))
	static USceneComponent* ResolveComponent(const FKzComponentSocketReference& Reference, UObject* Context);

	/**
	 * Resolves the reference and casts it to the specified Component Class.
	 * @param Reference The socket reference struct.
	 * @param Context The object initiating the query.
	 * @param ComponentClass The class to filter/cast the result to.
	 * @return The resolved component cast to the specific class, or nullptr if cast failed.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context", DeterminesOutputType = "ComponentClass"))
	static USceneComponent* ResolveComponentByClass(const FKzComponentSocketReference& Reference, UObject* Context, TSubclassOf<USceneComponent> ComponentClass);

	/**
	 * Resolves the reference to its Scene Component, along with the socket and offset it names, to hand on to whatever follows a socket.
	 * @param Reference The socket reference struct.
	 * @param Context The object initiating the query.
	 * @param Socket The socket on the component; None for the component itself.
	 * @param Offset The local offset, from RelativeLocation and RelativeRotation.
	 * @return The resolved component, or nullptr if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context"))
	static USceneComponent* ResolveComponentSocket(const FKzComponentSocketReference& Reference, UObject* Context, FName& Socket, FTransform& Offset);

	/**
	 * Calculates the World Transform of the referenced socket.
	 * @param Reference The socket reference struct.
	 * @param Context The object initiating the query.
	 * @return The calculated world transform. Returns Identity if the component cannot be resolved.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context", DisplayName = "Get World Transform"))
	static FTransform GetSocketTransform(const FKzComponentSocketReference& Reference, UObject* Context);

	/** Returns the World Location of the referenced socket (including offsets). */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context", DisplayName = "Get World Location"))
	static FVector GetSocketLocation(const FKzComponentSocketReference& Reference, UObject* Context);

	/**
	 * Attaches an Actor to the component and socket defined by the reference.
	 * @param Reference The target reference.
	 * @param ActorToAttach The actor that will be attached.
	 * @param Context The context to resolve the reference.
	 * @param AttachmentRules How to handle transform rules during attachment.
	 * @return True if attachment was successful.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context"))
	static bool AttachActorToReference(const FKzComponentSocketReference& Reference, AActor* ActorToAttach, UObject* Context, EAttachmentRule AttachmentRules = EAttachmentRule::SnapToTarget);

	/**
	 * Converts this static reference into a runtime Transform Source.
	 * @param Reference The socket reference struct.
	 * @param Context The object initiating the query.
	 * @return A constructed TransformSource containing the resolved component pointer and socket data.
	 */
	UFUNCTION(BlueprintPure, Category = "KzLib|SocketReference", meta = (DefaultToSelf = "Context", DisplayName = "To Transform Source"))
	static FKzTransformSource ToTransformSource(const FKzComponentSocketReference& Reference, UObject* Context);

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

	// === Object ===

	/**
	 * Copies properties from Source to Target.
	 * Only properties with matching names and types are copied.
	 * @param Source          The object to copy values FROM.
	 * @param Target          The object to copy values TO.
	 * @param bCopyTransients If true, properties marked as Transient will also be copied.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|System|Reflection", meta = (AdvancedDisplay = "bCopyTransients"))
	static void CopyObjectProperties(UObject* Source, UObject* Target, bool bCopyTransients = false);

	// === Components ===

	/**
	 * Tries to find a component of the specified class on the provided Actor.
	 * If not found, and the Actor is a Pawn, it tries to find the component on its Controller.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Components", meta = (DefaultToSelf = "Target", DeterminesOutputType = "ComponentClass"))
	static UActorComponent* FindComponentInActorOrController(AActor* Target, TSubclassOf<UActorComponent> ComponentClass);

	/**
	 * Tries to find a component of the specified class on the provided Controller.
	 * If not found it tries to find the component on its Pawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "KzLib|Components", meta = (DefaultToSelf = "Target", DeterminesOutputType = "ComponentClass"))
	static UActorComponent* FindComponentInControllerOrPawn(AController* Target, TSubclassOf<UActorComponent> ComponentClass);

	/**
	 * Templated version for C++ usage.
	 * Searches in the Actor first, then in the Controller if the Actor is a Pawn.
	 */
	template <typename T>
	static T* FindComponentInActorOrController(AActor* Target)
	{
		if (!Target)
		{
			return nullptr;
		}

		// Try Actor
		T* FoundComp = Target->FindComponentByClass<T>();
		if (FoundComp)
		{
			return FoundComp;
		}

		// Try Controller (if Pawn)
		if (APawn* Pawn = Cast<APawn>(Target))
		{
			if (AController* Controller = Pawn->GetController())
			{
				return Controller->FindComponentByClass<T>();
			}
		}

		return nullptr;
	}

	/**
	 * Templated version for C++ usage.
	 * Searches in the Controller first, then in the Pawn.
	 */
	template <typename T>
	static T* FindComponentInControllerOrPawn(AController* Target)
	{
		if (!Target)
		{
			return nullptr;
		}

		// Try Actor
		T* FoundComp = Target->FindComponentByClass<T>();
		if (FoundComp)
		{
			return FoundComp;
		}

		// Try Pawn
		if (APawn* Pawn = Target->GetPawn())
		{
			return Pawn->FindComponentByClass<T>();
		}

		return nullptr;
	}
};

// Inline implementations
#if KZ_KISMET_SYSTEM_INLINE_ENABLED
#include "KzSystemLibrary.inl"
#endif
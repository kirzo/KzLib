// Copyright 2025 kirzo

#include "KzSystemLibrary.h"
#include "Misc/KzTransformSource.h"
#include "Components/KzComponentSocketReference.h"

#pragma once

#if KZ_KISMET_SYSTEM_INLINE_ENABLED
#define KZ_SYSTEM_FORCEINLINE FORCEINLINE
#else
#define KZ_SYSTEM_FORCEINLINE // nothing
#endif

// === FKzTransformSource ===

KZ_SYSTEM_FORCEINLINE bool UKzSystemLibrary::IsValid(const FKzTransformSource& Source) { return Source.IsValid(); }

KZ_SYSTEM_FORCEINLINE FVector							UKzSystemLibrary::Conv_KzTransformSourceToVector(const FKzTransformSource& Source) { return Source.GetLocation(); }
KZ_SYSTEM_FORCEINLINE FRotator						UKzSystemLibrary::Conv_KzTransformSourceToRotator(const FKzTransformSource& Source) { return Source.GetRotation(); }
KZ_SYSTEM_FORCEINLINE FTransform					UKzSystemLibrary::Conv_KzTransformSourceToTransform(const FKzTransformSource& Source) { return Source.GetTransform(); }

KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_VectorToKzTransformSource(const FVector& Vector) { return FKzTransformSource(Vector); }
KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_RotatorToKzTransformSource(const FRotator& Rotator) { return FKzTransformSource(Rotator); }
KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_TransformToKzTransformSource(const FTransform& Source) { return FKzTransformSource(Source); }
KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_ActorToKzTransformSource(const AActor* Actor) { return FKzTransformSource(Actor, FVector::ZeroVector); }
KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_SceneComponentToKzTransformSource(const USceneComponent* Component, const FName SocketName) { return FKzTransformSource(Component, SocketName, FVector::ZeroVector); }
KZ_SYSTEM_FORCEINLINE FKzTransformSource	UKzSystemLibrary::Conv_KzComponentSocketReferenceToKzTransformSource(const FKzComponentSocketReference& ComponentRef, const UObject* ContextObject) { return ComponentRef.ToTransformSource(ContextObject); }

#undef KZ_SYSTEM_FORCEINLINE
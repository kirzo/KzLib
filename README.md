<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/KzLib_banner.jpg" alt="KzLib Banner" width="512">
</p>

<h1 align="center">KzLib</h1>

<p align="center">
  <em>A modular utility library for Unreal Engine — math, geometry, spatial structures, data-driven workflows, ECS, and editor tooling.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Unreal%20Engine-5.x-blue?logo=unrealengine" alt="Unreal Engine 5.x" />
  <img src="https://img.shields.io/badge/language-C%2B%2B20-00599C?logo=c%2B%2B" alt="C++20" />
  <img src="https://img.shields.io/badge/Blueprint-ready-orange" alt="Blueprint ready" />
  <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="MIT License" />
  <img src="https://img.shields.io/github/stars/kirzo/KzLib?style=social" alt="GitHub stars" />
</p>

---

## Table of Contents

- [Overview](#overview)
- [Modules](#modules)
- [Feature Tour](#feature-tour)
  - [Math & Accumulators](#math--accumulators)
  - [Geometry & Shapes](#geometry--shapes)
  - [Collision: Raycasts & GJK](#collision-raycasts--gjk)
  - [Spatial Structures](#spatial-structures)
  - [Containers](#containers)
  - [Data-Driven Database](#data-driven-database)
  - [Component & Transform References](#component--transform-references)
  - [Spline Tools](#spline-tools)
  - [Actors & Grouping](#actors--grouping)
  - [Serialization](#serialization)
  - [ECS](#ecs)
  - [Shaders](#shaders)
  - [Editor Tooling](#editor-tooling)
- [Requirements](#requirements)
- [Installation](#installation)
- [Repository Layout](#repository-layout)
- [Usage Examples](#usage-examples)
- [Related Projects](#related-projects)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

---

## Overview

**KzLib** is an open-source utility plugin for **Unreal Engine 5** that provides a curated set of low-level building blocks usable from both **C++** and **Blueprint**: math primitives, a unified geometric shape system, fast analytical raycasts, GJK convex collision, spatial acceleration structures, generational handles, a data-driven database backed by `FInstancedPropertyBag`, a lightweight ECS, and a sizeable editor toolkit (custom property pickers, asset editors, validators, custom K2 nodes…).

It is intentionally **modular** and **dependency-free** beyond Unreal itself, so you can drop it into any project without dragging in unrelated systems. KzLib is the foundation for larger frameworks I maintain, including:

- **[ScriptableFramework](https://github.com/kirzo/ScriptableFramework)** — a data-driven gameplay framework.
- **[Axon Physics](https://kirzo.dev/axon-physics/)** — a custom simulation/physics layer.

If you build advanced gameplay, simulation, or tooling on top of UE, KzLib gives you the kind of primitives the engine *almost* provides — but cleaner, safer, and more composable.

---

## Modules

KzLib ships as a multi-module Unreal plugin so you only pay for what you use:

| Module             | Type          | Purpose                                                                                          |
|--------------------|---------------|--------------------------------------------------------------------------------------------------|
| **`KzLib`**        | Runtime       | Core: math, geometry, collision, spatial, containers, database, components, serialization, etc. |
| **`KzLibECS`**     | Runtime       | Lightweight ECS (Entity, Storage, Registry, View, System) built on `THandleArray`.              |
| **`KzLibEditor`**  | Editor        | Property customizations, asset editors, component visualizers, validators, Slate widgets.        |
| **`KzLibUncooked`**| UncookedOnly  | Custom K2 (Blueprint) nodes that drive wildcard-typed pins (e.g. `EvaluateDatabaseAsset`).      |

The runtime modules are dependency-free outside of Unreal. The editor modules only load in editor builds and never enter cooked builds.

---

## Feature Tour

### Math & Accumulators

- **`FKzMath`** — pure C++ helpers (`GetHorizontalAngle`, `GetVerticalAngleDifference`).
- **`FKzVectorAccumulator`** — running (optionally weighted) average of `FVector` with proper validity tracking and operator overloads (`+=`, implicit `FVector` conversion).
- **`FKzQuatAccumulator`** — same idea for `FQuat`, robust against opposite-hemisphere quaternions through reference alignment.
- **`Kz::Random`** — Box–Muller Gaussian sampling for floats and vectors, with optional `FRandomStream` overloads.
- **`UKzMathLibrary`** — Blueprint surface for all of the above plus quaternion-log / rotation-vector conversions and quat/rotator transform helpers.

### Geometry & Shapes

A unified, polymorphic shape system designed around `FInstancedStruct`:

- **`FKzShape`** — abstract base. Subclasses implement bounds, closest point, point intersection, support point (for GJK), `Inflate` / `Scale`, debug draw and scene-proxy draw.
- **`FKzSphere`, `FKzBox`, `FKzCapsule`, `FKzCylinder`** — concrete shapes with operator overloads (`shape + inflation`, `shape * scale`).
- **`FKzShapeInstance`** — type-erased wrapper. Supports `Make<T>(...)`, `As<T>()`, `IsA<T>()`, `IntersectsPoint`, `GetBoundingBox`, `GetSupportPoint`, conversion to/from `FCollisionShape`.
- **`Kz::Geom`** — free namespace with bounds, closest-point, point-inside tests for each primitive, plus polygon helpers (`SimplifyPolygon`, `IsPointInPolygon2D`, `GetRandomPointInPolygon2D`, `DistanceToLine`).
- **`Kz::Geom::Sample`** — point sampling: AABB / oriented box vertices, sphere cardinals, sphyl rings, and a **Fibonacci sphere** distribution.
- **`UKzShapeComponent`** — primitive component with its own scene proxy that renders any `FKzShapeInstance` (wireframe + optional translucent solid), respecting selection state and component scaling.

### Collision: Raycasts & GJK

- **`Kz::Raycast`** — analytical, allocation-free raycasts against `Sphere`, `AABB`, `OBB`, `Capsule`, `Cylinder`. **No dependency on Unreal collision** — perfect for custom physics pipelines.
- **`Kz::GJK`**:
  - `Intersect(ShapeA, posA, rotA, ShapeB, posB, rotB)` — convex–convex intersection.
  - `Raycast(...)` — generic ray-vs-convex (with conservative advancement). When a shape implements its own analytical raycast, the GJK ray uses the fast path automatically.
- **`FKzHitResult`** — POD-style result struct with `NetSerialize`, `ToString`, `ToHitResult` for interop with `FHitResult`. Has a `BlueprintBreakHitResult` thunk so you can break it like the engine's hit result.
- **`UKzGeomLibrary`** — Blueprint nodes: `RayIntersectsShape/Sphere/Box/Capsule/Cylinder` and `LineIntersects*` variants, all with optional debug-draw overloads driven by `EDrawDebugTrace`.

### Spatial Structures

Both structures use a *Semantics* policy class that defines `GetBoundingBox`, `GetElementId`, `GetElementPosition`, `IsValid`, and (optionally) `GetShape` / `GetElementRotation`. They support an optional `Validator` lambda for runtime filtering (collision channels, teams, etc.).

- **`Kz::TOctree<Element, Semantics, bAllowMultiNode>`** — loose octree with:
  - Configurable max depth, min elements per node, and looseness.
  - Multi-node insertion (default) so elements straddling cells are queryable from both sides.
  - `Build`, `Raycast`, two `Query` overloads (`FBox` or `FKzShapeInstance`), and `DebugDraw`.
- **`Kz::TSpatialHashGrid<Element, Semantics>`** — sparse, *unbounded* hash grid:
  - 21-bit-per-axis packed key (~±1M cells).
  - `Insert` / `Remove` / `Remove(by previous bounds for O(1) removal)`.
  - **DDA voxel traversal** raycast — visits cells front-to-back with proper early-out.
  - Box and shape queries, plus debug draw.

### Containers

- **`Kz::THandleArray<T, Handle, Allocator>`** — generational, dense storage with stable handles (slot + generation):
  - **O(1)** `Add` / `Remove` / `Find` / `IsValid` / `Contains`.
  - Reuses freed slots automatically, bumps generation to invalidate stale handles.
  - Auto-assigns `Handle` member if your element type has one (`if constexpr (requires {...})`).
  - `RemoveAfter(Handle, Predicate)` to run code on the element before destruction.
  - Forward, const, reverse, and const-reverse iterators (range-for ready).
- **`FKzHandle`** — default lightweight handle (`int32 Index, int32 Generation`) with `IsValid`, `GetTypeHash`, `ToString` and `operator bool`.
- **`Kz::TPriorityStack<Entry, bKeepLastElement, Key, bCanContainDuplicates>`** — max-heap with **stable insertion** for equal priorities (LIFO via sequence counter). Find/remove by key, sorted retrieval, predicate filters, iterators.
- **`CKzContainer` concept** — C++20 concept that the spatial structures use to accept any iterable + `Num()` + `IsEmpty()` + `Empty()` / `Reset()` container.

### Data-Driven Database

A type-safe schema-validated database for tag-driven gameplay data, built on `FInstancedPropertyBag`:

- **`FKzParamDef`** — name + type definition (any UPROPERTY-compatible type, plus arrays). Compile-time helpers (`Init<T>()`, `Make<T>()`) and runtime conversion to `FPropertyBagPropertyDesc`.
- **`KzPropertyBag::Get<T> / Set<T> / Add<T>`** — generic type-mapped helpers covering bool, integer types, floating-point, name/string/text, enums, structs, raw `UObject*`, `TObjectPtr`, soft object/class pointers.
- **`FKzDatabase`** — schema (`FKzParamDef Type`) + array of `FKzDatabaseItem` (ID + tag container + payload). Templated `AddOrUpdateItem<T>()` validates type compatibility (with child-class polymorphism for objects).
- **`FKzDatabaseQuery`** — gameplay-tag query with `RequireTags` (must-have), `IgnoreTags` (must-not-have), `OptionalTags` (heuristic scoring), and `FGameplayTagQuery` (advanced expression). Sorts results by score.
- **`UKzDatabaseAsset`** — `UPrimaryDataAsset` wrapper with **inheritance** (`ParentDatabase`). Editor checks for self-reference, circular dependencies, and type mismatches.
- **`UKzDatabaseComponent`** — central registry on an actor. Auto-builds an O(1) type→asset map at `BeginPlay`. Templated `Resolve<T>(Query, OutValue)`.
- **`UKzDatabaseLibrary`** — Blueprint surface with **wildcard-pin** custom thunks (`AddDatabaseItem`, `Get/SetDatabaseItemValue`, `FindBestMatch`) so designers see the correct pin type per database.

### Component & Transform References

- **`FKzComponentReference`** — robust reference to a component by name, supports `OverrideActor`, **child-actor path syntax** (`MyChildActor.InnerMesh`), and BP property fallback. Survives renames better than raw pointers.
- **`FKzComponentSocketReference`** — extends the above with `SocketName`, `RelativeLocation`, `RelativeRotation`. Includes `GetSocketTransform(Context, OutTransform)` and `ToTransformSource(Context)`.
- **`FKzTransformSource`** — abstract source of a world transform: `Actor`, `SceneComponent + Socket`, or `Literal`. Net-serializable, equality-comparable, with implicit conversions to `FVector` / `FRotator` / `FTransform`. `BlueprintAutocast` operators wired up everywhere.

### Spline Tools

- **`AKzSplineActor`** — drop-in actor that contains a single `USplineComponent`. Provides a `GroundSpline()` callable that line-traces every spline point downward and snaps it to static geometry.
- **`UKzSplineAreaComponent`** — closed-loop spline that defines a 2D polygonal area: adaptive sampling (`SplineToPolygon`), polygon simplification, point-inside test (with AABB rejection), random point sampling, and closest-point-on-area.
- **`AKzAreaNetwork`** — composes an additive base spline area with `FKzAreaModifier` entries (`Add` / `Subtract`) to build CSG-style shapes from multiple `UKzSplineAreaComponent`s. `IsPointInside` and rejection-sampling `GetRandomPointInside`.
- **`UKzSplineFollowerComponent`** — moves any `SceneComponent` along a `USplineComponent`. Supports closed-loop, ping-pong, and one-shot modes; replicated `Speed` and `CurrentDistance`; events for `OnDirectionChanged`, `OnSplineEndReached`, `OnSplineLoopCompleted`; injects kinematic velocity for physics correctness; can preview-tick in the editor.

### Actors & Grouping

- **`AKzActorGroup`** — lightweight grouping actor: holds `TArray<AActor*> Actors`, locks scale to 1, draws a yellow billboard in editor and connection lines + bracket gizmo to all members via `FKzActorGroupVisualizer`.
- **`UKzRegistrySubsystem`** (world subsystem) — central registry of weak object pointers bucketed by class. Templated `RegisterItem<T>`, `UnregisterItem<T>`, `GetItems<T>` with automatic cleanup of GC'd entries.

### Serialization

- **`UKzSerializationLibrary`** — serialize and deserialize any `UObject`'s state to a `TArray<uint8>` using either:
  - `SaveGameProperties` — only properties marked `SaveGame` (vanilla UE behaviour).
  - `ModifiedProperties` — **delta serialization** vs. the CDO; only saves properties that differ from the archetype, with a per-property payload-size header so removed/renamed properties are safely skipped on load.
- **`SpawnActorWithData`** / **`SpawnActorFromInstance`** — deferred-spawn helpers that inject saved state *before* `BeginPlay` so the actor wakes up with the restored variables.
- **`ResetSaveGameProperties`** — restores all `CPF_SaveGame` properties to their CDO defaults.

### ECS

A small, fast ECS sitting on top of `THandleArray`:

- **`Kz::ECS::Entity`** — generational handle (inherits `FKzHandle`).
- **`Kz::ECS::Storage<T>`** — sparse-set storage: dense component array + dense entity array + sparse `EntityIndex → DenseIndex` table. O(1) Add / Remove (swap-back) / Get / Contains.
- **`Kz::ECS::Registry`** — owns entities and component storages, picks the smallest matching storage as the iteration base.
- **`Kz::ECS::TView<bConst, Included…, Excluded…>`** — typed iteration with `for (auto [e, pos, vel] : registry.View<Position, Velocity>())`, structured bindings, `Exclude<>()` chaining, and `ParallelForEach`.
- **`Kz::ECS::ISystem`** + **`Kz::ECS::SystemGroup`** — minimal scheduler so you can compose systems and update them in order with a single `DeltaTime`.

### Shaders

The plugin ships a `Shaders/` folder mapped to `/KzLib` at module startup so you can `#include` these from your materials and Niagara modules:

- **`MathLibrary.ush`** — quaternion construction from Euler, quaternion inversion, vector rotation, world-to-local / local-to-world helpers.
- **`Easings.ush`** — full set of Robert Penner-style easings (`Sine`, `Quad`, `Cubic`, `Quart`, `Quint`, `Expo`, `Circ`, `Back`, `Elastic`, `Bounce`, plus a tunable `SCurve`).
- **`SDFLibrary.ush`** — IQuilez SDF primitives (`sdSphere`, `sdBox`, `sdRoundBox`, `sdTorus`, `sdCappedTorus`, `sdLink`, `sdCylinder`, `sdCone`, `sdPlane`, `sdHexPrism`, `sdTriPrism`, `sdCapsule`, `sdVerticalCapsule`) and CSG ops (`opUnion`, `opSubtraction`, `opIntersection`) including smooth variants.
- **`CelShading.ush`** — depth + normal Laplacian filter for cel-shading and outline post-processes, with rotation and squash controls on the kernel.

### Editor Tooling

`KzLibEditor` is a sizeable editor toolkit, not just a couple of customizations:

- **Property customizations**:
  - `FKzParamDefCustomization` — pin-type selector backed by a custom `UKzParamDefSchema` that filters out unsupported types (Interfaces, Maps, Sets).
  - `FKzDatabaseCustomization` / `FKzDatabaseItemCustomization` — auto-syncs item schemas when the type changes; supports `FixedType` and `NoArrays` metadata.
  - `FKzComponentReferenceCustomization` — full component/socket picker that walks BP CDOs, traverses `ChildActorComponent`s with dot-notation, validates against `AllowedClasses` / `MustImplement` metadata, and shows inline warnings when a component or socket can't be resolved.
- **Asset workflow**:
  - `UKzDatabaseAssetFactory` — Content Browser entry under the "KzLib" category.
  - `FKzArrayAssetEditor` — generic, reusable asset toolkit with **four panels**: Asset Details, Array Stack (the property stack), Element Details, and **Validation**. Drives the editor experience for any asset whose primary content is a single array property.
  - `FKzPropertyStackRowCustomizer` — extension point for the array stack (custom leading/trailing widgets, display text, tooltip, background brush, GUID-based row resolution).
- **Reusable Slate widgets**:
  - `SKzPropertyStack` — drag-and-drop reorderable stack with search, filter, undo/redo, copy/cut/paste/delete commands, deep-copy support for `UObject` arrays via `FKzClipboardUtils`.
  - `SKzClassCombo` — class picker with category headings, search, and filter options.
  - `SKzParamDefSelector` — pin-type selector that consumes/produces `FKzParamDef` directly.
  - `SKzValidationPanel` — severity-filtered issue list with click-to-jump, used by `FKzArrayAssetEditor`.
- **Validation framework**:
  - `UKzAssetValidator` (Blueprintable, abstract) — implement `CanValidate` + `Validate` to add custom asset rules.
  - `FKzAssetValidationUtils::RunValidation(Asset)` — discovers all loaded validator subclasses via reflection and runs the matching ones.
  - `FKzValidationIssue` — severity (Info / Warning / Error), message, validator id, and optional GUID or array index for navigation.
- **Custom K2 nodes** (in `KzLibUncooked`):
  - `UK2Node_EvaluateDatabaseAsset` — node that exposes a `UKzDatabaseAsset` field and dynamically rebuilds its **Result** pin to match the asset's `FKzParamDef`. Validates type drift between the asset and the node.
  - `UK2Node_ResolveDatabaseQuery` — wildcard-out variant: the result pin's type is **inferred from whatever you connect to it**, then routed to the right typed thunk on `UKzDatabaseLibrary`.
  - `KzLib::Editor::PinTypeFromDef` / `DefFromPinType` — round-trip helpers between `FEdGraphPinType` and `FKzParamDef`.
- **Component visualizers**:
  - `FKzActorGroupVisualizer` — connection lines + bracketed bounding box around all grouped actors.
- **Style & infrastructure**:
  - `TKzEditorStyle_Base<T>` — CRTP base for plugin Slate styles (singleton, plugin-relative resource paths, helpers for class icons / thumbnails).
  - `FKzLibEditorModule_Base` — base module class with templated registration helpers (`RegisterAssetTypeAction`, `RegisterPropertyLayout`, `RegisterClassLayout`, `RegisterComponentVisualizer`) so subclasses just declare what they need.

---

## Requirements

- **Unreal Engine 5.x** with C++20 enabled (the codebase uses C++20 concepts and `requires` expressions).
- A C++-enabled project (the plugin contains C++ source modules).
- A toolchain capable of compiling UE plugins:
  - **Windows** — Visual Studio 2022 with the *Game development with C++* workload.
  - **macOS** — Xcode (latest stable supported by your UE version).
  - **Linux** — Clang as configured by Epic for your engine version.

> ℹ️ KzLib has **no third-party dependencies**. It only relies on Unreal Engine modules (`Core`, `CoreUObject`, `Engine`, `GameplayTags`, `Slate`, `SlateCore`, `RenderCore`, `Projects`, `NetCore`, plus editor-only modules in `KzLibEditor` / `KzLibUncooked`).

---

## Installation

KzLib is a standard Unreal Engine plugin. Pick whichever installation flow matches your workflow.

### Option A — Project plugin (recommended)

```bash
cd <YourProject>/Plugins
git clone https://github.com/kirzo/KzLib.git
```

Then:

1. Right-click your `.uproject` → **Generate Visual Studio project files**.
2. Open the project and build (or let the editor build it on first launch).
3. The plugin is enabled automatically; otherwise enable it via **Edit → Plugins → KzLib**.

### Option B — Git submodule

```bash
cd <YourProject>
git submodule add https://github.com/kirzo/KzLib.git Plugins/KzLib
git submodule update --init --recursive
```

### Option C — Engine plugin

Copy the `KzLib` folder into `<UnrealEngine>/Engine/Plugins/Marketplace/` (or any subfolder under `Engine/Plugins/`) to make it available to every project on that engine install.

### Using KzLib in your module

Add the runtime module dependency in your `*.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new[] {
    "Core",
    "CoreUObject",
    "Engine",
    "GameplayTags",
    "KzLib"
    // "KzLibECS"  // optional, only if you use the ECS
});
```

For shaders, include the mapped virtual path from your `.usf` / `.ush`:

```hlsl
#include "/KzLib/Easings.ush"
#include "/KzLib/SDFLibrary.ush"
```

---

## Repository Layout

```
KzLib/
├── Config/                 # Plugin .ini configuration
├── Content/                # Editor-side blueprint / asset content
├── Resources/              # Plugin icon and editor sprites (e.g. ActorGroup billboard)
├── Shaders/                # .ush files mapped to /KzLib at module startup
│   ├── CelShading.ush
│   ├── Easings.ush
│   ├── MathLibrary.ush
│   └── SDFLibrary.ush
├── Source/
│   ├── KzLib/              # Runtime module
│   │   ├── Public/
│   │   │   ├── Actors/             # KzActorGroup, KzAreaNetwork, KzSplineActor
│   │   │   ├── Collision/          # KzGJK, KzHitResult, KzRaycast
│   │   │   ├── Components/         # ComponentReference, Database, Shape, SplineArea, SplineFollower
│   │   │   ├── Concepts/           # KzContainer concept
│   │   │   ├── Containers/         # THandleArray, TPriorityStack
│   │   │   ├── Core/               # KzDatabase, KzDatabaseAsset, KzHandle, KzParamDef,
│   │   │   │                       # KzPropertyBag, KzRegistrySubsystem, KzTypes,
│   │   │   │                       # KzValidationTypes, KzMovementTypes
│   │   │   ├── Kismet/             # KzAppLibrary, KzDatabaseLibrary, KzGeomLibrary,
│   │   │   │                       # KzMathLibrary (+ .inl), KzRenderingLibrary,
│   │   │   │                       # KzSystemLibrary (+ .inl)
│   │   │   ├── Math/               # FKzMath, KzRandom, accumulators, geometry namespace, shapes
│   │   │   ├── Misc/               # KzEnumClassFlags, KzTransformSource
│   │   │   ├── Serialization/      # KzSerializationLibrary
│   │   │   └── Spatial/            # TOctree, TSpatialHashGrid (+ .inl)
│   │   └── Private/                # Implementation files (mirrors Public/)
│   ├── KzLibECS/           # Runtime ECS module
│   │   └── Public/
│   │       ├── KzEcsEntity.h
│   │       ├── KzEcsRegistry.h
│   │       ├── KzEcsStorage.h
│   │       ├── KzEcsSystem.h
│   │       ├── KzEcsSystemGroup.h
│   │       └── KzEcsView.h
│   ├── KzLibEditor/        # Editor module (customizations, asset editors, widgets, validation)
│   └── KzLibUncooked/      # UncookedOnly module (custom K2 nodes)
├── KzLib.uplugin
├── LICENSE                 # MIT
└── README.md
```

---

## Usage Examples

### Math accumulators

```cpp
FKzVectorAccumulator VecAvg;
FKzQuatAccumulator   QuatAvg;

for (const FMyType& Elem : Elements)
{
    if (!ShouldAverage(Elem)) continue;

    VecAvg  += Elem.Position;
    QuatAvg += Elem.Orientation;
}

if (!QuatAvg.IsValid()) return;

const FVector TargetPos = VecAvg;          // Implicit Resolve()
const FQuat   TargetRot = QuatAvg;         // Implicit Resolve() w/ hemisphere alignment
```

### Make a generic shape

```cpp
FKzShapeInstance Capsule  = FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
FKzShapeInstance Box      = FKzShapeInstance::Make(FKzBox(HalfSize));
FKzShapeInstance Inflated = Capsule + 5.0f;
```

### Capsule-vs-point intersection

```cpp
#include "Math/Geometry/KzGeometry.h"

const bool bInside = Kz::Geom::CapsuleIntersectsPoint(
    Center, FQuat::Identity, Radius, HalfHeight, SomePoint);
```

### Mathematical raycast (independent from UE collision)

```cpp
#include "Collision/KzRaycast.h"
#include "Collision/KzHitResult.h"

FKzHitResult Hit;
const bool bHit = Kz::Raycast::Box(
    Hit, Center, Rotation, HalfSize, RayStart, RayDir, MaxDist);
```

### GJK convex intersection

```cpp
#include "Collision/KzGJK.h"

const bool bIntersection = Kz::GJK::Intersect(
    ShapeA, PosA, RotA,
    ShapeB, PosB, RotB);
```

### Build and query an octree

```cpp
#include "Spatial/KzOctree.h"

Kz::TOctree<FMyElement, FMySemantics> Octree;
Octree.SetMaxDepth(6);
Octree.SetLooseness(1.2f);
Octree.Build(MyElements);

TArray<FMyElementId> OutIds;
Octree.Query(OutIds, QueryShape, QueryPos, QueryRot);
```

### Sparse hash grid raycast

```cpp
#include "Spatial/KzSpatialHashGrid.h"

Kz::TSpatialHashGrid<FMyElement, FMySemantics> Grid;
Grid.SetCellSize(500.0f);
Grid.Build(MyElements);

FKzHitResult Hit;
FMyElementId HitId;
if (Grid.Raycast(HitId, Hit, RayStart, RayDir, /*Length*/ 50000.f))
{
    // ...
}
```

### Stable handle container

```cpp
THandleArray<FMyType, FMyHandle> Registry;

const FMyHandle H = Registry.Add(FMyType{});
if (Registry.IsValid(H))
{
    Registry.FindChecked(H).DoSomething();
}
Registry.Remove(H); // generation bumped — old H is now invalid forever
```

### Database with tag-driven scoring

```cpp
FKzDatabaseQuery Query;
Query.RequireTags .AddTag(FGameplayTag::RequestGameplayTag("Class.Knight"));
Query.IgnoreTags  .AddTag(FGameplayTag::RequestGameplayTag("Status.Wounded"));
Query.OptionalTags.AddTag(FGameplayTag::RequestGameplayTag("Mood.Confident"));

UAnimMontage* Montage = nullptr;
if (DBComponent->Resolve<UAnimMontage*>(Query, Montage))
{
    PlayMontage(Montage);
}
```

### ECS view

```cpp
Kz::ECS::Registry Registry;

const auto E = Registry.CreateEntity();
Registry.AddComponent<FPosition>(E, {});
Registry.AddComponent<FVelocity>(E, {});

for (auto [Entity, Pos, Vel] : Registry.View<FPosition, FVelocity>())
{
    Pos.Value += Vel.Value * Dt;
}
```

### Abstract transform source

```cpp
FKzTransformSource Anchor(SomeActor);
const FVector Loc = Anchor.GetLocation();
const FQuat   Rot = Anchor.GetQuat();
```

### Component reference with child-actor path

```cpp
// In a UPROPERTY:
UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/Engine.SkeletalMeshComponent"))
FKzComponentSocketReference WeaponSocket;

// At runtime:
FTransform OutTransform;
if (WeaponSocket.GetSocketTransform(this, OutTransform))
{
    SpawnEffectAt(OutTransform);
}
```

### Custom asset editor in two lines

```cpp
RegisterAssetTypeAction<UMyAsset, FKzArrayAssetEditor>(
    KzAssetCategoryBit,
    INVTEXT("My Asset"),
    FColor::FromHex("#9B59B6"),
    /*SubMenus*/ {},
    /*ArrayPropertyName*/ FName("Items"),
    /*ItemName*/ INVTEXT("Item"));
```

---

## Related Projects

- **[ScriptableFramework](https://github.com/kirzo/ScriptableFramework)** — data-driven gameplay framework built on top of KzLib.
- **[Axon Physics](https://kirzo.dev/axon-physics/)** — custom simulation/physics system that uses KzLib's geometry, raycasting, GJK and spatial structures.

---

## Contributing

Contributions are welcome! If you'd like to help:

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/my-feature`.
3. Commit with clear messages and follow the existing code style (Unreal/Epic C++ conventions).
4. Open a Pull Request describing **what** changed and **why**.

For larger changes please open an issue first so we can align on direction before you invest time in the implementation.

---

## License

KzLib is released under the **MIT License**. See [LICENSE](LICENSE) for the full text.

You are free to use it in commercial and non-commercial projects.

---

## Author

Built and maintained by **[Kirzo](https://kirzo.dev/)**.

- 🌐 Website: [kirzo.dev](https://kirzo.dev/)
- 🐙 GitHub: [@kirzo](https://github.com/kirzo)

If KzLib helps your project, consider giving the repository a ⭐ — it really helps visibility.

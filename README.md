<p align="center">
  <img src="https://kirzo.dev/content/images/plugins/KzLib_banner.jpg" alt="KzLib Banner" width="512">
</p>

# KzLib

**KzLib** is a lightweight, **public**, and constantly growing utility library for **Unreal Engine**, focused on **mathematics, geometry, and generic programming**.

It’s designed to be **modular** and **dependency-free**, serving as a foundation for larger systems such as **[ScriptableFramework](https://github.com/kirzo/ScriptableFramework)**, [Axon Physics](https://kirzo.dev/axon-physics/), and other advanced simulation or gameplay frameworks.

---

## Key Features

- **Advanced Data & Reflection Utilities**
  (`FKzComponentSocketReference`, `FKzParamDef`, `KzPropertyBagHelpers`). Tools for robust socket referencing, defining dynamic parameters, and simplifying Unreal's `FInstancedPropertyBag` workflow.
- **Unified geometric shapes**  
  (`FKzShapeInstance`, `FKzSphere`, `FKzBox`, etc.) with full editor integration and debug visualization.  
- **Generic transform systems**  
  (`FKzTransformSource`, `FKzVectorAccumulator`, `FKzQuatAccumulator`) for handling positions and orientations abstractly.  
- **Safe handle containers**  
  (`THandleArray`) that eliminate dangling references while maintaining constant-time access.  
  - `THandleArray` is designed for large dynamic collections where elements can be created or destroyed at any time, providing **O(1)** insertion, removal, and access through stable handles.
- **Kz::Geom** — A collection of lightweight geometric utilities for spheres, boxes, capsules, cylinders, and more. Provides bounds computation, closest-point queries, intersection tests, and distance helpers.
- **Kz::Raycast** — Fast mathematical raycasts against primitive shapes. Completely independent from the Unreal Engine collision system and suitable for custom physics pipelines.
- **Kz::GJK** — Modern implementation of the GJK algorithm used for convex collision detection, minimal distance queries, and penetration depth/normal extraction.
- **Kz::TOctree** — A generic, high-performance templated octree supporting multi-node storage, dynamic depth/looseness control, and fast spatial queries. Integrates naturally with Kz::Raycast and Kz::Geom for broadphase+narrowphase workflows.
- **Full Blueprint integration**, including automatic conversions and debug utilities.
- **Open-source**, actively maintained, and steadily evolving with new tools and utilities.

---

## Usage Examples

### Creating a generic shape

```cpp
FKzShapeInstance Capsule = FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
FKzShapeInstance Box = FKzShapeInstance::Make(FKzBox(HalfSize));
```

### Performing a capsuple intersects point test

```cpp
#include "Math/Geometry/KzGeometry.h

bool bIntersects = Kz::Geom::CapsuleIntersectsPoint(Center, FQuat::Identity, Radius, HalfHeight, SomePoint);
```

### Raycasting

```cpp
#include "Collision/KzGeometry.h

bool bHit = Kz::Raycast::Box(Hit, Center, HalfSize, RayStart, RayDir, MaxDist);
```

### Using GJK intersection test

```cpp
#include "Collision/KzGJK.h

FKzShapeInstance ShapeA = ...;
FKzShapeInstance ShapeB = ...;

bool bIntersection = Kz::GJK::Intersect(ShapeA, PositionA, RotationA, ShapeB, PositionB, RotationB);
```

### Building and querying an octree

```cpp
#include "Spatial/KzOctree.h

Kz::TOctree<FMyElement, FMySemantics> Octree;
Octree.Build(MyElements);

FKzShapeInstance Shape = ...;

TArray<FMyElement> OutElements;
bool bFoundSome = Octree.Query(OutElements, Shape, Position, Orientation);
```

### Using a transform source (in Blueprint or C++):

```cpp
FKzTransformSource Anchor(SomeActor);
FVector Location = Anchor.GetLocation();
FQuat Rotation = Anchor.GetQuat();
```

### Safe handle container example:

```cpp
THandleArray<FMyType, FMyHandle> Registry;
FMyHandle Handle = Registry.Add(FMyType());
Registry.Remove(Handle); // No dangling references
```

### Using vector and quat accumulators:

```cpp
FKzVectorAccumulator VecAvg;
FKzQuatAccumulator QuatAvg;

for (const FMyType& Elem : SomeArray)
{
	if (!SomeCondition(Elem))
	{
		continue;
	}

	VecAvg += Elem.Position;
	QuatAvg += Elem.Orientation;
}

if (!QuatAvg.IsValid())
{
	continue;
}

const FVector TargetPosition = VecAvg;
const FQuat TargetRotation = QuatAvg;
```

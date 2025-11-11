# KzLib

**KzLib** is a lightweight, **public**, and constantly growing utility library for **Unreal Engine**, focused on **mathematics, geometry, and generic programming**.

It’s designed to be **modular** and **dependency-free**, serving as a foundation for larger systems such as [Axon Physics](https://kirzo.dev/axonphysics/) and other advanced simulation or gameplay frameworks.

---

## Key Features

- **Unified geometric shapes**  
  (`FKzShapeInstance`, `FKzSphere`, `FKzBox`, etc.) with full editor integration and debug visualization.  
- **Generic transform systems**  
  (`FKzTransformSource`, `FKzVectorAccumulator`, `FKzQuatAccumulator`) for handling positions and orientations abstractly.  
- **Safe handle containers**  
  (`THandleArray`) that eliminate dangling references while maintaining constant-time access.  
  - `THandleArray` is designed for large dynamic collections where elements can be created or destroyed at any time, providing **O(1)** insertion, removal, and access through stable handles.  
- **Full Blueprint integration**, including automatic conversions and debug utilities.
- **Open-source**, actively maintained, and steadily evolving with new tools and utilities.

---

## Usage Examples

### Creating a generic shape

```cpp
FKzShapeInstance Capsule = FKzShapeInstance::Make<FKzCapsule>(Radius, HalfHeight);
FKzShapeInstance Box = FKzShapeInstance::Make(FKzBox(HalfSize));
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

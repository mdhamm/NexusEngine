# Rotating Cube Implementation Summary

## ✅ What Was Created

### 1. **Proper Component Structure**
- `Mesh` and `Material` are now **shared resources** in `rendering/` folder (NOT components)
- Only structs with "Component" suffix are actual ECS components in `components/` folder

**File Structure:**
```
NexusEngine/src/
├── components/
│   ├── CameraComponent.h          ✅ ECS Component
│   ├── TransformComponent.h       ✅ ECS Component (NEW)
│   ├── RenderMeshComponent.h      ✅ ECS Component
│   └── RenderTextureComponent.h   ✅ ECS Component
└── rendering/
    ├── Mesh.h                      ✅ Shared Resource
    ├── Material.h                  ✅ Shared Resource
    ├── RenderResourceFactory.h     ✅ Factory
    └── RenderResourceFactory.cpp   ✅ Factory Implementation
```

### 2. **TransformComponent** (NEW)
A proper transform component using Diligent's math library:
- Position (float3)
- Rotation (float3 - Euler angles in radians)
- Scale (float3)
- `GetWorldMatrix()` method that computes the transformation matrix

### 3. **Material System**
Two built-in materials:
- **CreateDefaultMaterial()** - Lit material with simple directional lighting
- **CreateUnlitMaterial()** - Unlit material that visualizes normals (NEW)

Both materials:
- Create proper vertex and pixel shaders
- Set up graphics pipeline automatically
- Create constant buffers for transform matrices
- Support per-instance rendering

### 4. **Rendering Pipeline**
The `Scene::Render()` method now:
1. ✅ Calculates view and projection matrices
2. ✅ Iterates all `RenderMeshComponent` entities
3. ✅ Gets transform from `TransformComponent` (if present)
4. ✅ Updates constant buffers with World, ViewProj matrices
5. ✅ Binds pipeline state from material
6. ✅ Sets vertex/index buffers from mesh
7. ✅ Issues draw calls

### 5. **Rotating Cube Demo**
Updated `SampleGame/src/Game.cpp` to:
- Create a rotating cube using the factory
- Add `RotationSpeed` component for easy rotation control
- Create system that updates rotation every frame
- Use unlit material for visualization

## How It Works

### Resource Creation (One-Time)
```cpp
auto* factory = scene->GetResourceFactory();

// Create shared resources
Mesh* cubeMesh = factory->CreateCubeMesh();
Material* unlitMaterial = factory->CreateUnlitMaterial();
```

### Entity Creation
```cpp
auto cube = world.entity("RotatingCube")
    .set(TransformComponent{})
    .set(RotationSpeed{ 0.0f, 1.0f, 0.5f });

auto* renderMesh = cube.get_mut<RenderMeshComponent>();
renderMesh->mesh = cubeMesh;          // Reference shared mesh
renderMesh->material = unlitMaterial; // Reference shared material
renderMesh->visible = true;
```

### Per-Frame Rotation
```cpp
w.system<TransformComponent, RotationSpeed>("RotateObjects")
    .kind(flecs::OnUpdate)
    .iter([](flecs::iter& it, TransformComponent* transforms, RotationSpeed* speeds)
    {
        const float dt = static_cast<float>(it.delta_time());
        for (auto i : it)
        {
            transforms[i].rotation.y += speeds[i].y * dt;
            // ... other axes
        }
    });
```

### Rendering Flow
```
1. Scene::Render() calculates camera matrices (View, Projection)
2. For each RenderMeshComponent:
   a. Get TransformComponent → compute World matrix
   b. Update constant buffer: WorldViewProj = World * View * Projection
   c. Set pipeline state from Material
   d. Bind constant buffer to shader
   e. Set vertex/index buffers from Mesh
   f. Draw!
```

## Key Architectural Decisions

✅ **Resource Sharing**: One Mesh/Material can be used by many entities  
✅ **Clean Naming**: Only actual ECS components have "Component" suffix  
✅ **Proper Location**: Resources in `rendering/`, components in `components/`  
✅ **Pipeline Management**: Materials create and manage their own pipelines  
✅ **Automatic Constant Buffers**: Factory creates constant buffers with materials  
✅ **Transform-based**: Uses proper 4x4 matrices for positioning/rotation/scale  

## Built-in Materials

| Material | Description | Constant Buffer |
|----------|-------------|-----------------|
| `CreateDefaultMaterial()` | Simple directional lighting | 128 bytes (WorldViewProj + World) |
| `CreateUnlitMaterial()` | Visualizes normals as colors | 64 bytes (WorldViewProj) |

## Result

When you run the game, you should see:
🎮 A rotating cube that spins around Y and Z axes
🎨 Unlit shader visualizing normals as RGB colors
⚡ Proper pipeline and shader management
🔄 Smooth animation using delta time

## Next Steps

- [ ] Add more shapes (sphere, cylinder, etc.)
- [ ] Create textured materials
- [ ] Add lighting system
- [ ] Camera controls
- [ ] Material instancing for per-object colors

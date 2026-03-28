# NexusEngine Rendering System

## Overview

The NexusEngine rendering system separates concerns between geometry, materials, and rendering through a component-based architecture using Diligent Engine and Flecs ECS.

## Core Components

### 1. **MeshComponent** - Geometry Data
Contains only raw mesh geometry (vertices and indices).

```cpp
struct MeshComponent
{
    RefCntAutoPtr<IBuffer> vertexBuffer;
    RefCntAutoPtr<IBuffer> indexBuffer;
    Uint32 vertexCount;
    Uint32 indexCount;
    Uint32 vertexStride;
    PRIMITIVE_TOPOLOGY topology;
    std::string name;
};
```

### 2. **Material** - Shader and Pipeline State
Contains shaders, pipeline state, and material properties.

```cpp
struct Material
{
    RefCntAutoPtr<IShader> vertexShader;
    RefCntAutoPtr<IShader> pixelShader;
    RefCntAutoPtr<IPipelineState> pipelineState;
    RefCntAutoPtr<IShaderResourceBinding> shaderResourceBindingTemplate;
    
    // Textures
    RefCntAutoPtr<ITextureView> albedoTexture;
    RefCntAutoPtr<ITextureView> normalTexture;
    
    // Uniform buffers
    RefCntAutoPtr<IBuffer> materialConstantBuffer;
    
    // Properties
    std::string name;
    bool isTransparent;
    CULL_MODE cullMode;
    bool depthTestEnabled;
    bool depthWriteEnabled;
};
```

### 3. **RenderMeshComponent** - Renderable Entity
Combines a mesh with a material for rendering. This is what you attach to entities.

```cpp
struct RenderMeshComponent
{
    MeshComponent* mesh;        // Reference to shared mesh
    Material* material;         // Reference to shared material
    
    // Per-instance resources
    RefCntAutoPtr<IShaderResourceBinding> instanceSRB;
    RefCntAutoPtr<IBuffer> instanceConstantBuffer;
    
    // Flags
    bool visible;
    bool castShadows;
    bool receiveShadows;
    int renderLayer;
};
```

## Usage

### Basic Setup

```cpp
// Get the scene's resource factory
auto* factory = scene.GetResourceFactory();

// Create a mesh (shared resource)
MeshComponent* cubeMesh = factory->CreateCubeMesh();

// Create a material (shared resource)
Material* material = factory->CreateDefaultMaterial();

// Create an entity with RenderMesh
auto cubeEntity = scene.CreateEntity("MyCube");
auto* renderMesh = cubeEntity.get_mut<RenderMeshComponent>();
renderMesh->mesh = cubeMesh;
renderMesh->material = material;
renderMesh->visible = true;
```

### Built-in Shapes

```cpp
// Create basic shapes
MeshComponent* cube = factory->CreateCubeMesh();
MeshComponent* plane = factory->CreatePlaneMesh();
MeshComponent* sphere = factory->CreateSphereMesh(32); // 32 segments
```

### Custom Material with Inline Shaders

```cpp
const char* vertexShader = R"(
struct VSInput
{
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV : ATTRIB2;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

cbuffer Constants
{
    float4x4 ViewProj;
    float4x4 World;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), mul(World, ViewProj));
    PSIn.Color = VSIn.Normal * 0.5 + 0.5;
}
)";

const char* pixelShader = R"(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color, 1.0);
}
)";

// Define vertex layout matching your vertex structure
std::vector<LayoutElement> layout = {
    LayoutElement{0, 0, 3, VT_FLOAT32, False},  // Position
    LayoutElement{1, 0, 3, VT_FLOAT32, False},  // Normal
    LayoutElement{2, 0, 2, VT_FLOAT32, False}   // UV
};

// Create material
Material* customMat = factory->CreateMaterial(
    "MyMaterial",
    vertexShader,
    pixelShader,
    layout);
```

### Loading Shaders from Files

```cpp
auto* vs = factory->CreateShaderFromFile(
    "shaders/MyVertex.hlsl",
    "main",
    SHADER_TYPE_VERTEX,
    "MyVertexShader");

auto* ps = factory->CreateShaderFromFile(
    "shaders/MyPixel.hlsl",
    "main",
    SHADER_TYPE_PIXEL,
    "MyPixelShader");

// Create pipeline manually
auto* pso = factory->CreateGraphicsPipeline(
    "MyPipeline",
    vs,
    ps,
    layout,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    CULL_MODE_BACK,
    true,  // depth test
    true); // depth write
```

### Custom Mesh Creation

```cpp
struct Vertex
{
    float pos[3];
    float normal[3];
    float uv[2];
};

Vertex vertices[] = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}
};

Uint32 indices[] = { 0, 1, 2 };

Mesh* triangle = factory->CreateMesh(
    "Triangle",
    vertices,
    3,                    // vertex count
    sizeof(Vertex),       // vertex stride
    indices,
    3);                   // index count
```

### Sharing Resources Across Multiple Entities

```cpp
// Create shared resources once (Mesh and Material are NOT per-entity)
Mesh* sharedMesh = factory->CreateCubeMesh();
Material* sharedMaterial = factory->CreateDefaultMaterial();

// Create 100 cubes sharing the same mesh and material
for (int i = 0; i < 100; ++i)
{
    auto entity = scene.CreateEntity();
    auto* renderMesh = entity.get_mut<RenderMeshComponent>();
    renderMesh->mesh = sharedMesh;        // Shared!
    renderMesh->material = sharedMaterial; // Shared!
    renderMesh->visible = true;

    // Each entity can have unique transform (TODO: add TransformComponent)
}
```

### Per-Instance Data (Advanced)

```cpp
// Create per-instance constant buffer for unique per-object data
auto* renderMesh = entity.get_mut<RenderMeshComponent>();

renderMesh->instanceConstantBuffer = factory->CreateConstantBuffer(
    "InstanceConstants",
    sizeof(MyInstanceData));

// Clone the material's SRB for per-instance bindings
if (renderMesh->material->shaderResourceBindingTemplate)
{
    renderMesh->material->pipelineState->CreateShaderResourceBinding(
        &renderMesh->instanceSRB, 
        true);
}

// Update constant buffer with instance-specific data
// (This would typically be done per frame in a system)
auto* ctx = graphicsRenderer.m_gfx.m_ctx;
MapHelper<MyInstanceData> constants(ctx, renderMesh->instanceConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
*constants = myInstanceData;
```

## Predefined Vertex Formats

```cpp
// Position + Normal + UV
struct VertexPosNormalUV
{
    float position[3];
    float normal[3];
    float uv[2];
};

// Position + Color + UV
struct VertexPosColorUV
{
    float position[3];
    float color[4];
    float uv[2];
};
```

## RenderResourceFactory API

| Method | Description |
|--------|-------------|
| `CreateShaderFromSource()` | Create shader from inline HLSL string |
| `CreateShaderFromFile()` | Load shader from file |
| `CreateGraphicsPipeline()` | Create complete graphics pipeline state |
| `CreateMaterial()` | Create material with shaders and pipeline |
| `CreateDefaultMaterial()` | Create simple lit material |
| `CreateMesh()` | Create mesh from vertex/index data |
| `CreateCubeMesh()` | Create unit cube mesh |
| `CreatePlaneMesh()` | Create unit plane mesh |
| `CreateSphereMesh()` | Create UV sphere mesh |
| `CreateConstantBuffer()` | Create uniform/constant buffer |

## Architecture Benefits

1. **Resource Sharing**: Multiple entities can share the same mesh and material, reducing memory usage
2. **Separation of Concerns**: Geometry, materials, and rendering are cleanly separated
3. **Flexibility**: Easy to swap meshes or materials at runtime
4. **ECS Integration**: Works seamlessly with Flecs component system
5. **Diligent Engine Integration**: Direct access to Diligent's powerful rendering features

## Scene Rendering Flow

```
1. Scene::Render() iterates all cameras
2. For each camera, iterate all RenderMeshComponent entities
3. For each visible RenderMesh:
   - Set pipeline state from material
   - Bind shader resources (material or instance SRB)
   - Set vertex buffer from mesh
   - Set index buffer from mesh (if present)
   - Issue draw call (indexed or non-indexed)
```

## Next Steps / TODOs

- [ ] Add TransformComponent for positioning entities
- [ ] Add camera matrices to constant buffers
- [ ] Implement render-to-texture support
- [ ] Add texture loading utilities
- [ ] Implement material batching/sorting
- [ ] Add support for multiple render passes
- [ ] Implement shadow mapping
- [ ] Add support for skinned meshes

## Example Files

See `NexusEngine/src/examples/RenderingExamples.h` for complete working examples.

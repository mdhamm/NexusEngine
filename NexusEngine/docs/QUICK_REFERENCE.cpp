// Quick Reference: Creating Renderable Objects in NexusEngine

#include <Scene.h>
#include <components/TransformComponent.h>
#include <components/RenderMeshComponent.h>
#include <rendering/RenderResourceFactory.h>

// ==============================================================
// STEP 1: Get the scene and factory
// ==============================================================
NexusEngine::Scene* scene = engine.FindScene("MainScene");
auto* factory = scene->GetResourceFactory();

// ==============================================================
// STEP 2: Create shared resources (Mesh and Material)
// These can be reused across multiple entities!
// ==============================================================

// Built-in shapes
NexusEngine::Mesh* cube   = factory->CreateCubeMesh();
NexusEngine::Mesh* plane  = factory->CreatePlaneMesh();
NexusEngine::Mesh* sphere = factory->CreateSphereMesh(32);

// Built-in materials
NexusEngine::Material* unlitMat = factory->CreateUnlitMaterial();
NexusEngine::Material* litMat   = factory->CreateDefaultMaterial();

// ==============================================================
// STEP 3: Create an entity with RenderMeshComponent
// ==============================================================
auto entity = scene->m_world.entity("MyObject");

// Add transform (for positioning/rotation/scale)
entity.set(NexusEngine::TransformComponent{
    .position = {0.0f, 0.0f, 0.0f},
    .rotation = {0.0f, 0.0f, 0.0f},
    .scale = {1.0f, 1.0f, 1.0f}
});

// Add render mesh (references shared resources)
auto* renderMesh = entity.get_mut<NexusEngine::RenderMeshComponent>();
renderMesh->mesh = cube;
renderMesh->material = unlitMat;
renderMesh->visible = true;

// ==============================================================
// STEP 4 (Optional): Add rotation behavior
// ==============================================================

// Define a simple rotation component
struct RotationSpeed { float x, y, z; };

// Add to entity
entity.set(RotationSpeed{ 0.0f, 1.0f, 0.0f }); // Rotate 1 rad/sec around Y

// Create system to update rotation
scene->m_world.system<NexusEngine::TransformComponent, RotationSpeed>("Rotate")
    .kind(flecs::OnUpdate)
    .iter([](flecs::iter& it, 
             NexusEngine::TransformComponent* transforms, 
             RotationSpeed* speeds)
    {
        float dt = static_cast<float>(it.delta_time());
        for (auto i : it)
        {
            transforms[i].rotation.x += speeds[i].x * dt;
            transforms[i].rotation.y += speeds[i].y * dt;
            transforms[i].rotation.z += speeds[i].z * dt;
        }
    });

// ==============================================================
// That's it! The Scene::Render() will automatically:
// - Get the transform matrix
// - Update shader constant buffers
// - Bind the pipeline
// - Draw the mesh
// ==============================================================

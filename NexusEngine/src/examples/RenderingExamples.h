#pragma once
/*
 * Example Usage of RenderMesh System
 * 
 * This file demonstrates how to use the new rendering system with
 * RenderMeshComponent, Material, and MeshComponent.
 */

#include "Scene.h"
#include "components/RenderMeshComponent.h"
#include "rendering/Material.h"
#include "rendering/Mesh.h"
#include "rendering/RenderResourceFactory.h"

namespace NexusEngine
{
    namespace Example
    {
        // Example: Create a simple cube with default material
        void CreateCubeEntity(Scene& scene)
        {
            auto* factory = scene.GetResourceFactory();
            if (!factory)
                return;

            // Create mesh and material (these are shared resources, not components)
            Mesh* cubeMesh = factory->CreateCubeMesh();
            Material* defaultMaterial = factory->CreateDefaultMaterial();

            // Create entity with RenderMesh component
            auto cubeEntity = scene.CreateEntity("Cube");
            auto* renderMesh = cubeEntity.get_mut<RenderMeshComponent>();
            
            renderMesh->mesh = cubeMesh;
            renderMesh->material = defaultMaterial;
            renderMesh->visible = true;
            renderMesh->castShadows = true;
            renderMesh->receiveShadows = true;
        }

        // Example: Create multiple cubes sharing the same mesh and material
        void CreateMultipleCubes(Scene& scene, int count)
        {
            auto* factory = scene.GetResourceFactory();
            if (!factory)
                return;

            // Create shared resources once
            MeshComponent* cubeMesh = factory->CreateCubeMesh();
            Material* material = factory->CreateDefaultMaterial();

            // Create multiple entities
            for (int i = 0; i < count; ++i)
            {
                std::string name = "Cube_" + std::to_string(i);
                auto entity = scene.CreateEntity(name.c_str());
                
                auto* renderMesh = entity.get_mut<RenderMeshComponent>();
                renderMesh->mesh = cubeMesh;
                renderMesh->material = material;
                renderMesh->visible = true;
                
                // TODO: Set transform component to position cubes differently
            }
        }

        // Example: Create custom material with inline shaders
        void CreateCustomMaterialEntity(Scene& scene)
        {
            auto* factory = scene.GetResourceFactory();
            if (!factory)
                return;

            // Define custom vertex shader
            const char* customVS = R"(
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
    PSIn.Color = VSIn.Normal * 0.5 + 0.5; // Visualize normals as colors
}
)";

            // Define custom pixel shader
            const char* customPS = R"(
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

            // Define vertex layout
            std::vector<Diligent::LayoutElement> layout = {
                Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
                Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
                Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, Diligent::False}
            };

            // Create custom material
            Material* customMaterial = factory->CreateMaterial(
                "CustomMaterial",
                customVS,
                customPS,
                layout);

            // Create mesh
            MeshComponent* sphereMesh = factory->CreateSphereMesh(32);

            // Create entity
            auto entity = scene.CreateEntity("CustomMaterialSphere");
            auto* renderMesh = entity.get_mut<RenderMeshComponent>();
            renderMesh->mesh = sphereMesh;
            renderMesh->material = customMaterial;
            renderMesh->visible = true;
        }

        // Example: Create a plane with custom material
        void CreatePlane(Scene& scene)
        {
            auto* factory = scene.GetResourceFactory();
            if (!factory)
                return;

            MeshComponent* planeMesh = factory->CreatePlaneMesh();
            Material* material = factory->CreateDefaultMaterial();

            auto planeEntity = scene.CreateEntity("Plane");
            auto* renderMesh = planeEntity.get_mut<RenderMeshComponent>();
            renderMesh->mesh = planeMesh;
            renderMesh->material = material;
            renderMesh->visible = true;
        }
    }
} // namespace NexusEngine

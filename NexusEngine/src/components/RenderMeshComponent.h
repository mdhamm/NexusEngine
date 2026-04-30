#pragma once

#include "ComponentReflection.h"

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace NexusEngine
{
    struct Mesh;
    struct Material;

    // ECS component that binds a mesh and material to an entity for rendering.
    struct RenderMeshComponent
    {
        // Shared mesh geometry used by this entity.
        Mesh* mesh = nullptr;

        // Shared material used to render the mesh.
        Material* material = nullptr;

        // Optional per-instance shader resource binding.
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> instanceSRB;

        // Optional per-instance constant buffer.
        Diligent::RefCntAutoPtr<Diligent::IBuffer> instanceConstantBuffer;

        // Enables or disables rendering for this component.
        bool visible = true;

        // Enables shadow casting for this mesh.
        bool castShadows = true;

        // Enables receiving shadows for this mesh.
        bool receiveShadows = true;

        // Render layer used for filtering and ordering.
        int renderLayer = 0; // For sorting/filtering

        /// <summary>
        /// Creates the editor reflection descriptor for the render mesh component.
        /// </summary>
        /// <returns>The render mesh component descriptor.</returns>
        static ComponentDescriptor CreateDescriptor()
        {
            return ComponentDescriptor{
                "RenderMeshComponent",
                [](const flecs::entity& entity) { return entity.has<RenderMeshComponent>(); },
                [](flecs::entity entity) { entity.set(RenderMeshComponent{}); },
                [](const flecs::entity& entity)
                {
                    std::vector<ComponentPropertyDescriptor> properties;
                    const auto* renderMesh = entity.get<RenderMeshComponent>();
                    if (!renderMesh)
                    {
                        return properties;
                    }

                    properties.push_back(ComponentPropertyDescriptor{
                        "Mesh",
                        "string",
                        ComponentPropertyValueType::String,
                        true,
                        [renderMesh](const flecs::entity&) { return renderMesh->mesh ? std::string("Assigned") : std::string("None"); },
                        {} });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Material",
                        "string",
                        ComponentPropertyValueType::String,
                        true,
                        [renderMesh](const flecs::entity&) { return renderMesh->material ? std::string("Assigned") : std::string("None"); },
                        {} });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Visible",
                        "bool",
                        ComponentPropertyValueType::Bool,
                        false,
                        [renderMesh](const flecs::entity&) { return renderMesh->visible ? std::string("true") : std::string("false"); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RenderMeshComponent>())
                            {
                                editable->visible = text == "true";
                            }
                        } });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Cast Shadows",
                        "bool",
                        ComponentPropertyValueType::Bool,
                        false,
                        [renderMesh](const flecs::entity&) { return renderMesh->castShadows ? std::string("true") : std::string("false"); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RenderMeshComponent>())
                            {
                                editable->castShadows = text == "true";
                            }
                        } });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Receive Shadows",
                        "bool",
                        ComponentPropertyValueType::Bool,
                        false,
                        [renderMesh](const flecs::entity&) { return renderMesh->receiveShadows ? std::string("true") : std::string("false"); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RenderMeshComponent>())
                            {
                                editable->receiveShadows = text == "true";
                            }
                        } });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Render Layer",
                        "int",
                        ComponentPropertyValueType::Int,
                        false,
                        [renderMesh](const flecs::entity&) { return std::to_string(renderMesh->renderLayer); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RenderMeshComponent>())
                            {
                                editable->renderLayer = std::stoi(text);
                            }
                        } });
                    return properties;
                } };
        }
    };
} // namespace NexusEngine

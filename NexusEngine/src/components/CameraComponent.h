#pragma once

#include "ComponentReflection.h"
#include "RenderTextureComponent.h"

namespace NexusEngine
{
    // Camera component describing where a scene should render.
    struct CameraComponent
    {
        // Output target used by the camera.
        enum class Target
        {
            // Render to the main swap chain back buffer.
            SwapChain,

            // Render into a texture target.
            RenderTexture
        };

        // Output mode for the camera.
        Target m_target = Target::SwapChain;

        // Render texture used when targeting an off-screen surface.
        RenderTextureComponent* m_renderTexture = nullptr;

        // Camera sort priority. Higher values render later.
        int m_priority = 0;     // higher = rendered later (on top)

        /// <summary>
        /// Creates the editor reflection descriptor for the camera component.
        /// </summary>
        /// <returns>The camera component descriptor.</returns>
        static ComponentDescriptor CreateDescriptor()
        {
            return ComponentDescriptor{
                "CameraComponent",
                [](const flecs::entity& entity) { return entity.has<CameraComponent>(); },
                [](flecs::entity entity) { entity.set(CameraComponent{}); },
                [](const flecs::entity& entity)
                {
                    std::vector<ComponentPropertyDescriptor> properties;
                    const auto* camera = entity.get<CameraComponent>();
                    if (!camera)
                    {
                        return properties;
                    }

                    properties.push_back(ComponentPropertyDescriptor{
                        "Target",
                        "string",
                        ComponentPropertyValueType::String,
                        true,
                        [camera](const flecs::entity&)
                        {
                            return camera->m_target == CameraComponent::Target::SwapChain ? std::string("SwapChain") : std::string("RenderTexture");
                        },
                        {} });
                    properties.push_back(ComponentPropertyDescriptor{
                        "Priority",
                        "int",
                        ComponentPropertyValueType::Int,
                        false,
                        [camera](const flecs::entity&) { return std::to_string(camera->m_priority); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<CameraComponent>())
                            {
                                editable->m_priority = std::stoi(text);
                            }
                        } });
                    return properties;
                } };
        }
    };
} // namespace NexusEngine

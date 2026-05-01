#pragma once

#include "ComponentReflection.h"

namespace NexusEngine
{
    // Fly camera controller component that allows controlling a camera with free movement and rotation, typically used for editor cameras.
    struct FlyCameraComponent
    {
        float m_moveSpeed = 12.0f;
        float m_lookSensitivity = 0.003f;
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        bool m_isRotationInitialized = false;

        /// <summary>
        /// Creates the editor reflection descriptor for the fly camera controller component.
        /// </summary>
        /// <returns>The fly camera controller component descriptor.</returns>
        static ComponentDescriptor CreateDescriptor()
        {
            return NexusEngine::ComponentDescriptor{
                "FlyCameraComponent",
                [](const flecs::entity& entity) { return entity.has<FlyCameraComponent>(); },
                [](flecs::entity entity) { entity.set(FlyCameraComponent{}); },
                [](const flecs::entity& entity)
                {
                    std::vector<NexusEngine::ComponentPropertyDescriptor> properties;
                    const auto* controller = entity.get<FlyCameraComponent>();
                    if (!controller)
                    {
                        return properties;
                    }

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Move Speed",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [controller](const flecs::entity&) { return NexusEngine::FormatComponentFloat(controller->m_moveSpeed); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<FlyCameraComponent>())
                            {
                                editable->m_moveSpeed = std::stof(text);
                            }
                        } });

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Look Sensitivity",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [controller](const flecs::entity&) { return NexusEngine::FormatComponentFloat(controller->m_lookSensitivity); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<FlyCameraComponent>())
                            {
                                editable->m_lookSensitivity = std::stof(text);
                            }
                        } });

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Yaw",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [controller](const flecs::entity&) { return NexusEngine::FormatComponentFloat(controller->m_yaw); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<FlyCameraComponent>())
                            {
                                editable->m_yaw = std::stof(text);
                            }
                        } });

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Pitch",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [controller](const flecs::entity&) { return NexusEngine::FormatComponentFloat(controller->m_pitch); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<FlyCameraComponent>())
                            {
                                editable->m_pitch = std::stof(text);
                            }
                        } });

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Rotation Initialized",
                        "bool",
                        NexusEngine::ComponentPropertyValueType::Bool,
                        false,
                        [controller](const flecs::entity&) { return controller->m_isRotationInitialized ? std::string("true") : std::string("false"); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<FlyCameraComponent>())
                            {
                                editable->m_isRotationInitialized = text == "true";
                            }
                        } });

                    return properties;
                } };
        }
    };
} // namespace NexusEngine
#pragma once

#include <ComponentReflection.h>

namespace SampleGame
{
    struct FlyCameraControllerComponent
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
        static NexusEngine::ComponentDescriptor CreateDescriptor()
        {
            return NexusEngine::ComponentDescriptor{
                "FlyCameraControllerComponent",
                [](const flecs::entity& entity) { return entity.has<FlyCameraControllerComponent>(); },
                [](flecs::entity entity) { entity.set(FlyCameraControllerComponent{}); },
                [](const flecs::entity& entity)
                {
                    std::vector<NexusEngine::ComponentPropertyDescriptor> properties;
                    const auto* controller = entity.get<FlyCameraControllerComponent>();
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
                            if (auto* editable = target.get_mut<FlyCameraControllerComponent>())
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
                            if (auto* editable = target.get_mut<FlyCameraControllerComponent>())
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
                            if (auto* editable = target.get_mut<FlyCameraControllerComponent>())
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
                            if (auto* editable = target.get_mut<FlyCameraControllerComponent>())
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
                            if (auto* editable = target.get_mut<FlyCameraControllerComponent>())
                            {
                                editable->m_isRotationInitialized = text == "true";
                            }
                        } });
                    return properties;
                } };
        }
    };

    struct RotationSpeed
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;

        /// <summary>
        /// Creates the editor reflection descriptor for the rotation speed component.
        /// </summary>
        /// <returns>The rotation speed component descriptor.</returns>
        static NexusEngine::ComponentDescriptor CreateDescriptor()
        {
            return NexusEngine::ComponentDescriptor{
                "RotationSpeed",
                [](const flecs::entity& entity) { return entity.has<RotationSpeed>(); },
                [](flecs::entity entity) { entity.set(RotationSpeed{}); },
                [](const flecs::entity& entity)
                {
                    std::vector<NexusEngine::ComponentPropertyDescriptor> properties;
                    const auto* rotationSpeed = entity.get<RotationSpeed>();
                    if (!rotationSpeed)
                    {
                        return properties;
                    }

                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "X",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [rotationSpeed](const flecs::entity&) { return NexusEngine::FormatComponentFloat(rotationSpeed->m_x); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RotationSpeed>())
                            {
                                editable->m_x = std::stof(text);
                            }
                        } });
                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Y",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [rotationSpeed](const flecs::entity&) { return NexusEngine::FormatComponentFloat(rotationSpeed->m_y); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RotationSpeed>())
                            {
                                editable->m_y = std::stof(text);
                            }
                        } });
                    properties.push_back(NexusEngine::ComponentPropertyDescriptor{
                        "Z",
                        "float",
                        NexusEngine::ComponentPropertyValueType::Float,
                        false,
                        [rotationSpeed](const flecs::entity&) { return NexusEngine::FormatComponentFloat(rotationSpeed->m_z); },
                        [](const flecs::entity& target, const std::string& text)
                        {
                            if (auto* editable = target.get_mut<RotationSpeed>())
                            {
                                editable->m_z = std::stof(text);
                            }
                        } });
                    return properties;
                } };
        }
    };
} // namespace SampleGame

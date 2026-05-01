#pragma once

#include <ComponentReflection.h>

namespace SampleGame
{
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

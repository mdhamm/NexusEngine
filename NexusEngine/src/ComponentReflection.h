#pragma once

#include <flecs.h>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace NexusEngine
{
    enum class ComponentPropertyValueType
    {
        Bool,
        Int,
        Float,
        String
    };

    struct ComponentPropertyDescriptor
    {
        std::string m_name;
        std::string m_typeName;
        ComponentPropertyValueType m_valueType = ComponentPropertyValueType::String;
        bool m_isReadOnly = false;
        std::function<std::string(const flecs::entity&)> m_getValue;
        std::function<void(const flecs::entity&, const std::string&)> m_setValue;
    };

    struct ComponentDescriptor
    {
        std::string m_name;
        std::function<bool(const flecs::entity&)> m_hasComponent;
        std::function<void(const flecs::entity&)> m_addComponent;
        std::function<std::vector<ComponentPropertyDescriptor>(const flecs::entity&)> m_getProperties;
    };

    inline std::string FormatComponentFloat(float value)
    {
        std::ostringstream stream;
        stream.precision(4);
        stream << std::fixed << value;
        return stream.str();
    }

    class ComponentReflectionRegistry
    {
    public:
        /// <summary>
        /// Returns the singleton component reflection registry.
        /// </summary>
        /// <returns>The shared component reflection registry.</returns>
        static ComponentReflectionRegistry& Instance();

        /// <summary>
        /// Registers or replaces a component descriptor.
        /// </summary>
        /// <param name="descriptor">Descriptor to add to the registry.</param>
        void RegisterDescriptor(ComponentDescriptor descriptor);

        /// <summary>
        /// Returns all registered component descriptors.
        /// </summary>
        /// <returns>The registered component descriptors.</returns>
        const std::vector<ComponentDescriptor>& GetDescriptors() const;

    private:
        std::vector<ComponentDescriptor> m_descriptors;
    };
} // namespace NexusEngine

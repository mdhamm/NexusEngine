#pragma once

#include <flecs.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace NexusEngine
{
    enum class AssetType
    {
        Mesh,
        Material,
        Scene
    };

    using TypeId = std::type_index;

    // TODO: move this to a separate header and rename to AssetReferenceComponent or something like that
    struct AssetReference
    {
        std::string m_guid;
    };

    struct FieldMetadata
    {
        std::string m_name;
        size_t m_offset = 0;
        size_t m_size = 0;
        TypeId m_type = typeid(void);
        std::optional<AssetType> m_assetType;
        std::unordered_map<std::string, int> m_namedValues;

        /// <summary>
        /// Returns a mutable pointer to the field inside a component instance.
        /// </summary>
        /// <param name="componentData">Component instance address.</param>
        /// <returns>Pointer to the field storage.</returns>
        void* GetPointer(void* componentData) const
        {
            return static_cast<void*>(static_cast<char*>(componentData) + m_offset);
        }

        /// <summary>
        /// Returns a read-only pointer to the field inside a component instance.
        /// </summary>
        /// <param name="componentData">Component instance address.</param>
        /// <returns>Pointer to the field storage.</returns>
        const void* GetPointer(const void* componentData) const
        {
            return static_cast<const void*>(static_cast<const char*>(componentData) + m_offset);
        }
    };

    struct FieldView
    {
        void* m_data = nullptr;
        const FieldMetadata* m_meta = nullptr;
    };

    struct ComponentMetadata
    {
        std::string m_name;
        TypeId m_componentType = typeid(void);
        std::vector<FieldMetadata> m_fields;
        std::function<bool(const flecs::entity&)> m_hasComponent;
        std::function<void(const flecs::entity&)> m_addComponent;
        std::function<const void*(const flecs::entity&)> m_getComponent;
        std::function<void*(const flecs::entity&)> m_getMutableComponent;
        std::function<void(const flecs::entity&, void*)> m_afterApply;
    };

    struct ComponentView
    {
        void* m_data = nullptr;
        const ComponentMetadata* m_meta = nullptr;
    };

    class IComponentConsumer
    {
    public:
        virtual ~IComponentConsumer() = default;
        virtual void consume(const ComponentView& component) = 0;
    };

    template<typename TResult>
    class IFieldHandler
    {
    public:
        virtual ~IFieldHandler() = default;
        virtual TResult Handle(const FieldView& field) const = 0;
    };

    template<typename TResult>
    class TypedFieldDispatcher
    {
    public:
        template<typename TField>
        void Register(std::shared_ptr<IFieldHandler<TResult>> handler)
        {
            m_handlers[std::type_index(typeid(TField))] = std::move(handler);
        }

        const IFieldHandler<TResult>* Find(TypeId type) const
        {
            auto it = m_handlers.find(type);
            return it != m_handlers.end() ? it->second.get() : nullptr;
        }

    private:
        std::unordered_map<TypeId, std::shared_ptr<IFieldHandler<TResult>>> m_handlers;
    };

    class MetadataRegistry
    {
    public:
        class FieldBuilder;
        class ComponentBuilder;

        /// <summary>
        /// Returns the shared metadata registry instance.
        /// </summary>
        /// <returns>The shared metadata registry.</returns>
        static MetadataRegistry& Instance();

        /// <summary>
        /// Starts metadata registration for a component type.
        /// </summary>
        /// <typeparam name="T">Component type to register.</typeparam>
        /// <param name="name">Component display name.</param>
        /// <returns>A builder used to register component fields.</returns>
        template<typename T>
        ComponentBuilder component(std::string name)
        {
            ComponentMetadata& metadata = m_components[std::type_index(typeid(T))];
            metadata.m_name = std::move(name);
            metadata.m_componentType = std::type_index(typeid(T));
            metadata.m_fields.clear();
            return ComponentBuilder(metadata);
        }

        /// <summary>
        /// Returns metadata for a component type.
        /// </summary>
        /// <typeparam name="T">Component type to query.</typeparam>
        /// <returns>The component metadata, or null when unregistered.</returns>
        template<typename T>
        const ComponentMetadata* Get() const
        {
            auto it = m_components.find(std::type_index(typeid(T)));
            return it != m_components.end() ? &it->second : nullptr;
        }

        /// <summary>
        /// Returns metadata for a component type id.
        /// </summary>
        /// <param name="type">Component type id to query.</param>
        /// <returns>The component metadata, or null when unregistered.</returns>
        const ComponentMetadata* Get(TypeId type) const;

        /// <summary>
        /// Returns metadata for a component name.
        /// </summary>
        /// <param name="name">Component name to query.</param>
        /// <returns>The component metadata, or null when unregistered.</returns>
        const ComponentMetadata* FindByName(const std::string& name) const;

        const std::unordered_map<TypeId, ComponentMetadata>& GetAll() const
        {
            return m_components;
        }

        template<typename T>
        void SetEntityAccessors()
        {
            auto it = m_components.find(std::type_index(typeid(T)));
            if (it == m_components.end())
            {
                return;
            }

            it->second.m_hasComponent = [](const flecs::entity& entity)
            {
                return entity.has<T>();
            };

            it->second.m_addComponent = [](const flecs::entity& entity)
            {
                flecs::entity mutableEntity = entity;
                mutableEntity.set(T{});
            };

            it->second.m_getComponent = [](const flecs::entity& entity) -> const void*
            {
                return entity.get<T>();
            };

            it->second.m_getMutableComponent = [](const flecs::entity& entity) -> void*
            {
                flecs::entity mutableEntity = entity;
                return mutableEntity.get_mut<T>();
            };
        }

        template<typename T, typename TFunc>
        void SetAfterApply(TFunc&& func)
        {
            auto it = m_components.find(std::type_index(typeid(T)));
            if (it == m_components.end())
            {
                return;
            }

            it->second.m_afterApply = [handler = std::forward<TFunc>(func)](const flecs::entity& entity, void* componentData)
            {
                handler(entity, *static_cast<T*>(componentData));
            };
        }

    private:
        std::unordered_map<TypeId, ComponentMetadata> m_components;
    };

    class MetadataRegistry::FieldBuilder
    {
    public:
        explicit FieldBuilder(FieldMetadata& metadata)
            : m_metadata(&metadata)
        {
        }

        /// <summary>
        /// Tags a field as referencing a specific asset category.
        /// </summary>
        /// <param name="assetType">Semantic asset type of the field.</param>
        /// <returns>The current field builder.</returns>
        FieldBuilder& assetType(AssetType assetType)
        {
            m_metadata->m_assetType = assetType;
            return *this;
        }

        FieldBuilder& enumValue(std::string name, int value)
        {
            m_metadata->m_namedValues.emplace(std::move(name), value);
            return *this;
        }

    private:
        FieldMetadata* m_metadata = nullptr;
    };

    class MetadataRegistry::ComponentBuilder
    {
    public:
        template<typename T, typename TField>
        class FieldRegistration;

        explicit ComponentBuilder(ComponentMetadata& metadata)
            : m_metadata(&metadata)
        {
        }

        /// <summary>
        /// Registers metadata for a component field.
        /// </summary>
        /// <typeparam name="T">Component type that owns the field.</typeparam>
        /// <typeparam name="TField">Field type.</typeparam>
        /// <param name="name">Field name.</param>
        /// <param name="fieldPointer">Pointer-to-member identifying the field.</param>
        /// <returns>A field builder for additional semantic tags.</returns>
        template<typename T, typename TField>
        class FieldRegistration
        {
        public:
            FieldRegistration(ComponentBuilder& builder, FieldMetadata& metadata)
                : m_builder(&builder)
                , m_fieldBuilder(metadata)
            {
            }

            FieldRegistration& assetType(AssetType assetType)
            {
                m_fieldBuilder.assetType(assetType);
                return *this;
            }

            FieldRegistration& enumValue(std::string name, int value)
            {
                m_fieldBuilder.enumValue(std::move(name), value);
                return *this;
            }

            template<typename TOwner, typename TNextField>
            FieldRegistration<TOwner, TNextField> field(std::string name, TNextField TOwner::* fieldPointer)
            {
                return m_builder->field(std::move(name), fieldPointer);
            }

            template<typename TNextField>
            FieldRegistration<void, TNextField> field(std::string name, size_t offset)
            {
                return m_builder->field<TNextField>(std::move(name), offset);
            }

        private:
            ComponentBuilder* m_builder = nullptr;
            FieldBuilder m_fieldBuilder;
        };

        template<typename T, typename TField>
        FieldRegistration<T, TField> field(std::string name, TField T::* fieldPointer)
        {
            FieldMetadata fieldMetadata;
            fieldMetadata.m_name = std::move(name);
            fieldMetadata.m_offset = static_cast<size_t>(reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T const volatile*>(0)->*fieldPointer)));
            fieldMetadata.m_size = sizeof(TField);
            fieldMetadata.m_type = std::type_index(typeid(TField));
            m_metadata->m_fields.push_back(std::move(fieldMetadata));
            return FieldRegistration<T, TField>(*this, m_metadata->m_fields.back());
        }

        template<typename TField>
        FieldRegistration<void, TField> field(std::string name, size_t offset)
        {
            FieldMetadata fieldMetadata;
            fieldMetadata.m_name = std::move(name);
            fieldMetadata.m_offset = offset;
            fieldMetadata.m_size = sizeof(TField);
            fieldMetadata.m_type = std::type_index(typeid(TField));
            m_metadata->m_fields.push_back(std::move(fieldMetadata));
            return FieldRegistration<void, TField>(*this, m_metadata->m_fields.back());
        }

    private:
        ComponentMetadata* m_metadata = nullptr;
    };

    template<typename T>
    struct ComponentMeta;

    /// <summary>
    /// Registers a component with Flecs and the semantic metadata registry.
    /// </summary>
    /// <typeparam name="T">Component type to register.</typeparam>
    /// <param name="world">World receiving Flecs meta registration.</param>
    /// <param name="registry">Semantic metadata registry.</param>
    template<typename T>
    void RegisterComponent(flecs::world& world, MetadataRegistry& registry)
    {
        ComponentMeta<T>::Register(world, registry);
        registry.SetEntityAccessors<T>();
    }
} // namespace NexusEngine

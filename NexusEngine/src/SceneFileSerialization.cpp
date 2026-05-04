#include "SceneFileSerialization.h"

#include "ComponentAccess.h"
#include "MetadataRegistry.h"
#include "components/EditorOnlyComponent.h"
#include "components/RenderMeshComponent.h"
#include "components/TransformComponent.h"
#include "rendering/RenderResourceFactory.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace NexusEngine
{
    namespace
    {
        using json = nlohmann::json;

        struct SceneComponentData
        {
            std::string m_typeId;
            std::vector<std::pair<std::string, std::string>> m_fields;
        };

        struct SceneEntityData
        {
            std::uint64_t m_id = 0;
            std::string m_name;
            std::uint64_t m_parentId = 0;
            std::vector<SceneComponentData> m_components;
        };

        struct SceneDocumentData
        {
            std::string m_name;
            std::vector<SceneEntityData> m_entities;
        };

        const char* GetSceneFileSuffix()
        {
            return ".nscene";
        }

        std::string Trim(std::string_view text)
        {
            size_t start = 0;
            size_t end = text.size();
            while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0)
            {
                ++start;
            }

            while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
            {
                --end;
            }

            return std::string(text.substr(start, end - start));
        }

        std::string ToLowerAscii(std::string_view text)
        {
            std::string lowered(text);
            std::transform(
                lowered.begin(),
                lowered.end(),
                lowered.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            return lowered;
        }

        std::string GenerateGuid()
        {
            std::random_device randomDevice;
            std::mt19937 generator(randomDevice());
            std::uniform_int_distribution<int> distribution(0, 255);

            std::array<unsigned char, 16> bytes{};
            for (unsigned char& value : bytes)
            {
                value = static_cast<unsigned char>(distribution(generator));
            }

            bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
            bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

            std::ostringstream stream;
            stream << std::hex;
            for (size_t index = 0; index < bytes.size(); ++index)
            {
                if (index == 4 || index == 6 || index == 8 || index == 10)
                {
                    stream << '-';
                }

                stream.width(2);
                stream.fill('0');
                stream << static_cast<int>(bytes[index]);
            }

            return stream.str();
        }

        const ComponentMetadata* FindComponentMetadata(const std::string& typeId)
        {
            return MetadataRegistry::Instance().FindByName(typeId);
        }

        void AssignDefaultRenderResources(Scene& scene)
        {
            RenderResourceFactory* factory = scene.GetResourceFactory();
            if (!factory)
            {
                return;
            }

            Mesh* cubeMesh = nullptr;
            Material* unlitMaterial = nullptr;

            scene.m_world.each<RenderMeshComponent>(
                [&](flecs::entity, RenderMeshComponent& renderMesh)
                {
                    if (renderMesh.mesh && (renderMesh.material || !renderMesh.m_materialAssetPath.empty()))
                    {
                        return;
                    }

                    if (!cubeMesh)
                    {
                        cubeMesh = factory->CreateCubeMesh();
                    }

                    if (!unlitMaterial)
                    {
                        unlitMaterial = factory->CreateUnlitMaterial();
                    }

                    if (!renderMesh.mesh)
                    {
                        renderMesh.mesh = cubeMesh;
                    }

                    if (!renderMesh.material && renderMesh.m_materialAssetPath.empty())
                    {
                        renderMesh.material = unlitMaterial;
                    }
                });
        }

        json FieldToJson(const FieldView& field)
        {
            return json{
                { "name", std::string(field.m_name) },
                { "type", GetFieldTypeLabel(field) },
                { "typeId", GetFieldTypeLabel(field) },
                { "value", ReadFieldText(field).value_or(std::string{}) },
                { "readOnly", IsFieldReadOnly(field) }
            };
        }

        json ComponentToJson(const ComponentMetadata& metadata, const flecs::entity& entity)
        {
            json componentJson = {
                { "type", "Component" },
                { "typeId", metadata.m_name },
                { "fields", json::array() }
            };

            const std::optional<ComponentView> componentView = CreateEntityComponentView(metadata, entity);
            if (!componentView)
            {
                return componentJson;
            }

            ForEachField(
                *componentView,
                [&](const FieldView& field)
                {
                    componentJson["fields"].push_back(FieldToJson(field));
                });

            return componentJson;
        }

        json SceneToJson(const Scene& scene, std::string_view assetGuid)
        {
            const std::string trimmedGuid = Trim(assetGuid);
            json sceneJson = {
                { "guid", trimmedGuid.empty() ? GenerateGuid() : trimmedGuid },
                { "name", scene.m_name },
                { "entities", json::array() }
            };

            scene.m_world.each<TransformComponent>(
                [&](flecs::entity entity, TransformComponent&)
                {
                    if (!entity.is_valid() || !entity.is_alive() || entity.has<EditorOnlyComponent>())
                    {
                        return;
                    }

                    json entityJson = {
                        { "id", static_cast<std::uint64_t>(entity.id()) },
                        { "name", entity.name() ? std::string(entity.name()) : std::string{} },
                        { "parentId", entity.parent().is_valid() ? static_cast<std::uint64_t>(entity.parent().id()) : static_cast<std::uint64_t>(0) },
                        { "components", json::array() }
                    };

                    for (const auto& [type, metadata] : MetadataRegistry::Instance().GetAll())
                    {
                        (void)type;
                        if (!metadata.m_hasComponent || !metadata.m_hasComponent(entity))
                        {
                            continue;
                        }

                        entityJson["components"].push_back(ComponentToJson(metadata, entity));
                    }

                    sceneJson["entities"].push_back(std::move(entityJson));
                });

            return sceneJson;
        }

        bool TryReadSceneDocument(const json& sceneJson, SceneDocumentData& sceneDocument)
        {
            if (!sceneJson.is_object())
            {
                return false;
            }

            sceneDocument = {};
            sceneDocument.m_name = sceneJson.value("name", std::string{});

            const auto entitiesIt = sceneJson.find("entities");
            if (entitiesIt == sceneJson.end() || !entitiesIt->is_array())
            {
                return true;
            }

            sceneDocument.m_entities.reserve(entitiesIt->size());
            for (const json& entityJson : *entitiesIt)
            {
                if (!entityJson.is_object())
                {
                    continue;
                }

                SceneEntityData entityData;
                entityData.m_id = entityJson.value("id", static_cast<std::uint64_t>(0));
                entityData.m_name = entityJson.value("name", std::string{});
                entityData.m_parentId = entityJson.value("parentId", static_cast<std::uint64_t>(0));

                if (const auto componentsIt = entityJson.find("components"); componentsIt != entityJson.end() && componentsIt->is_array())
                {
                    entityData.m_components.reserve(componentsIt->size());
                    for (const json& componentJson : *componentsIt)
                    {
                        if (!componentJson.is_object())
                        {
                            continue;
                        }

                        SceneComponentData componentData;
                        componentData.m_typeId = componentJson.value("typeId", componentJson.value("name", std::string{}));

                        const json* fieldsJson = nullptr;
                        if (const auto fieldsIt = componentJson.find("fields"); fieldsIt != componentJson.end() && fieldsIt->is_array())
                        {
                            fieldsJson = &(*fieldsIt);
                        }
                        else if (const auto propertiesIt = componentJson.find("properties"); propertiesIt != componentJson.end() && propertiesIt->is_array())
                        {
                            fieldsJson = &(*propertiesIt);
                        }

                        if (fieldsJson)
                        {
                            componentData.m_fields.reserve(fieldsJson->size());
                            for (const json& fieldJson : *fieldsJson)
                            {
                                if (!fieldJson.is_object())
                                {
                                    continue;
                                }

                                componentData.m_fields.emplace_back(
                                    fieldJson.value("name", std::string{}),
                                    fieldJson.value("value", std::string{}));
                            }
                        }

                        entityData.m_components.push_back(std::move(componentData));
                    }
                }

                sceneDocument.m_entities.push_back(std::move(entityData));
            }

            return true;
        }

        bool TryReadJsonFile(const std::filesystem::path& filePath, json& document)
        {
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open())
            {
                return false;
            }

            try
            {
                file >> document;
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool WriteJsonFile(const std::filesystem::path& filePath, const json& document)
        {
            std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                return false;
            }

            file << document.dump(4) << '\n';
            return file.good();
        }
    }

    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath)
    {
        return SaveSceneToFile(scene, filePath, ReadSceneFileGuid(filePath));
    }

    bool SaveSceneToFile(const Scene& scene, const std::filesystem::path& filePath, std::string_view assetGuid)
    {
        return WriteJsonFile(filePath, SceneToJson(scene, assetGuid));
    }

    bool LoadSceneFromFile(Scene& scene, const std::filesystem::path& filePath)
    {
        json document;
        if (!TryReadJsonFile(filePath, document))
        {
            return false;
        }

        SceneDocumentData sceneDocument;
        if (!TryReadSceneDocument(document, sceneDocument))
        {
            return false;
        }

        scene.m_name = sceneDocument.m_name.empty() ? filePath.stem().string() : sceneDocument.m_name;

        std::vector<flecs::entity> existingEntities;
        scene.m_world.each<TransformComponent>(
            [&](flecs::entity entity, TransformComponent&)
            {
                if (entity.is_valid() && entity.is_alive())
                {
                    existingEntities.push_back(entity);
                }
            });

        for (const flecs::entity& entity : existingEntities)
        {
            scene.DestroyEntity(entity);
        }

        struct PendingEntity
        {
            flecs::entity m_entity;
            std::uint64_t m_parentId = 0;
            std::vector<SceneComponentData> m_components;
        };

        std::unordered_map<std::uint64_t, PendingEntity> pendingEntities;
        pendingEntities.reserve(sceneDocument.m_entities.size());

        for (const SceneEntityData& entityData : sceneDocument.m_entities)
        {
            PendingEntity pendingEntity;
            pendingEntity.m_entity = entityData.m_name.empty()
                ? scene.CreateEntity()
                : scene.CreateEntity(entityData.m_name.c_str());
            pendingEntity.m_parentId = entityData.m_parentId;
            pendingEntity.m_components = entityData.m_components;
            pendingEntities.emplace(entityData.m_id, std::move(pendingEntity));
        }

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            for (const SceneComponentData& componentData : pendingEntity.m_components)
            {
                const ComponentMetadata* metadata = FindComponentMetadata(componentData.m_typeId);
                if (metadata && metadata->m_addComponent)
                {
                    metadata->m_addComponent(pendingEntity.m_entity);
                }
            }
        }

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            for (const SceneComponentData& componentData : pendingEntity.m_components)
            {
                const ComponentMetadata* metadata = FindComponentMetadata(componentData.m_typeId);
                if (!metadata)
                {
                    continue;
                }

                for (const auto& [fieldName, fieldValue] : componentData.m_fields)
                {
                    (void)WriteFieldText(*metadata, pendingEntity.m_entity, fieldName, fieldValue);
                }
            }
        }

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            if (pendingEntity.m_parentId == 0)
            {
                continue;
            }

            const auto parentIt = pendingEntities.find(pendingEntity.m_parentId);
            if (parentIt != pendingEntities.end())
            {
                pendingEntity.m_entity.child_of(parentIt->second.m_entity);
            }
        }

        AssignDefaultRenderResources(scene);
        return true;
    }

    bool CreateEmptySceneFile(const std::filesystem::path& filePath, std::string_view sceneName)
    {
        return CreateEmptySceneFile(filePath, sceneName, {});
    }

    bool CreateEmptySceneFile(const std::filesystem::path& filePath, std::string_view sceneName, std::string_view assetGuid)
    {
        std::error_code errorCode;
        if (const std::filesystem::path directoryPath = filePath.parent_path(); !directoryPath.empty())
        {
            std::filesystem::create_directories(directoryPath, errorCode);
            if (errorCode)
            {
                return false;
            }
        }

        json sceneJson = {
            { "guid", Trim(assetGuid).empty() ? GenerateGuid() : Trim(assetGuid) },
            { "name", std::string(sceneName) },
            { "entities", json::array({
                json{
                    { "id", static_cast<std::uint64_t>(1) },
                    { "name", "Cube" },
                    { "parentId", static_cast<std::uint64_t>(0) },
                    { "components", json::array({
                        json{
                            { "type", "Component" },
                            { "typeId", "TransformComponent" },
                            { "fields", json::array({
                                json{{ "name", "Position.X" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Position.Y" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Position.Z" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Rotation.X" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Rotation.Y" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Rotation.Z" }, { "type", "float" }, { "typeId", "float" }, { "value", "0.0000" }, { "readOnly", false }},
                                json{{ "name", "Scale.X" }, { "type", "float" }, { "typeId", "float" }, { "value", "1.0000" }, { "readOnly", false }},
                                json{{ "name", "Scale.Y" }, { "type", "float" }, { "typeId", "float" }, { "value", "1.0000" }, { "readOnly", false }},
                                json{{ "name", "Scale.Z" }, { "type", "float" }, { "typeId", "float" }, { "value", "1.0000" }, { "readOnly", false }}
                            }) }
                        },
                        json{
                            { "type", "Component" },
                            { "typeId", "RenderMeshComponent" },
                            { "fields", json::array({
                                json{{ "name", "Material Asset" }, { "type", "AssetReference" }, { "typeId", "AssetReference" }, { "value", "" }, { "readOnly", false }},
                                json{{ "name", "Visible" }, { "type", "bool" }, { "typeId", "bool" }, { "value", "true" }, { "readOnly", false }},
                                json{{ "name", "Cast Shadows" }, { "type", "bool" }, { "typeId", "bool" }, { "value", "true" }, { "readOnly", false }},
                                json{{ "name", "Receive Shadows" }, { "type", "bool" }, { "typeId", "bool" }, { "value", "true" }, { "readOnly", false }},
                                json{{ "name", "Render Layer" }, { "type", "int" }, { "typeId", "int" }, { "value", "0" }, { "readOnly", false }}
                            }) }
                        }
                    }) }
                }
            }) }
        };

        return WriteJsonFile(filePath, sceneJson);
    }

    std::string ReadSceneFileGuid(const std::filesystem::path& filePath)
    {
        json document;
        if (!TryReadJsonFile(filePath, document) || !document.is_object())
        {
            return {};
        }

        return Trim(document.value("guid", std::string{}));
    }

    std::string EnsureSceneFileGuid(const std::filesystem::path& filePath)
    {
        json document;
        if (!TryReadJsonFile(filePath, document) || !document.is_object())
        {
            return {};
        }

        const std::string existingGuid = Trim(document.value("guid", std::string{}));
        if (!existingGuid.empty())
        {
            return existingGuid;
        }

        const std::string generatedGuid = GenerateGuid();
        document["guid"] = generatedGuid;
        return WriteJsonFile(filePath, document) ? generatedGuid : std::string{};
    }

    bool IsSceneFilePath(const std::filesystem::path& filePath)
    {
        return ToLowerAscii(filePath.extension().string()) == GetSceneFileSuffix();
    }
} // namespace NexusEngine

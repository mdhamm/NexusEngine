#include "EditorSceneSerializer.h"

#include <ComponentReflection.h>
#include <Scene.h>
#include <components/TransformComponent.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

#include <unordered_map>
#include <vector>

namespace NexusEditor
{
    namespace
    {
        const QString& GetSceneFileSuffix()
        {
            static const QString s_sceneFileSuffix = QStringLiteral(".nscene");
            return s_sceneFileSuffix;
        }

        const NexusEngine::ComponentDescriptor* FindComponentDescriptor(const std::string& componentName)
        {
            for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
            {
                if (descriptor.m_name == componentName)
                {
                    return &descriptor;
                }
            }

            return nullptr;
        }
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath)
    {
        return SaveSceneToFile(scene, filePath, ReadSceneFileGuid(filePath));
    }

    bool SaveSceneToFile(const NexusEngine::Scene& scene, const QString& filePath, const QString& assetGuid)
    {
        QJsonObject sceneObject;
        sceneObject["guid"] = assetGuid.trimmed().isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces)
            : assetGuid.trimmed();
        sceneObject["name"] = QString::fromStdString(scene.m_name);

        QJsonArray entitiesArray;
        scene.m_world.each<NexusEngine::TransformComponent>(
            [&](flecs::entity entity, NexusEngine::TransformComponent&)
            {
                if (!entity.is_valid() || !entity.is_alive())
                {
                    return;
                }

                QJsonObject entityObject;
                entityObject["id"] = static_cast<qint64>(entity.id());
                entityObject["name"] = QString::fromUtf8(entity.name() ? entity.name() : "");

                const flecs::entity parent = entity.parent();
                entityObject["parentId"] = parent.is_valid() ? static_cast<qint64>(parent.id()) : 0;

                QJsonArray componentsArray;
                for (const auto& descriptor : NexusEngine::ComponentReflectionRegistry::Instance().GetDescriptors())
                {
                    if (!descriptor.m_hasComponent || !descriptor.m_hasComponent(entity))
                    {
                        continue;
                    }

                    QJsonObject componentObject;
                    componentObject["name"] = QString::fromStdString(descriptor.m_name);

                    QJsonArray propertiesArray;
                    const auto properties = descriptor.m_getProperties
                        ? descriptor.m_getProperties(entity)
                        : std::vector<NexusEngine::ComponentPropertyDescriptor>{};

                    for (const auto& property : properties)
                    {
                        QJsonObject propertyObject;
                        propertyObject["name"] = QString::fromStdString(property.m_name);
                        propertyObject["type"] = QString::fromStdString(property.m_typeName);
                        propertyObject["value"] = property.m_getValue
                            ? QString::fromStdString(property.m_getValue(entity))
                            : QString{};
                        propertiesArray.append(propertyObject);
                    }

                    componentObject["properties"] = propertiesArray;
                    componentsArray.append(componentObject);
                }

                entityObject["components"] = componentsArray;
                entitiesArray.append(entityObject);
            });

        sceneObject["entities"] = entitiesArray;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        const QJsonDocument document(sceneObject);
        file.write(document.toJson(QJsonDocument::Indented));
        return file.error() == QFileDevice::NoError;
    }

    bool LoadSceneFromFile(NexusEngine::Scene& scene, const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            return false;
        }

        const QJsonObject sceneObject = document.object();
        scene.m_name = sceneObject["name"].toString(QFileInfo(filePath).completeBaseName()).toStdString();

        std::vector<flecs::entity> existingEntities;
        scene.m_world.each<NexusEngine::TransformComponent>(
            [&](flecs::entity entity, NexusEngine::TransformComponent&)
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
            qint64 m_parentId = 0;
            QJsonArray m_components;
        };

        std::unordered_map<qint64, PendingEntity> pendingEntities;

        const QJsonArray entitiesArray = sceneObject["entities"].toArray();
        for (const QJsonValue& entityValue : entitiesArray)
        {
            if (!entityValue.isObject())
            {
                continue;
            }

            const QJsonObject entityObject = entityValue.toObject();
            const QString entityName = entityObject["name"].toString();
            const qint64 originalId = entityObject["id"].toInteger();

            PendingEntity pendingEntity;
            pendingEntity.m_entity = entityName.isEmpty()
                ? scene.CreateEntity()
                : scene.CreateEntity(entityName.toUtf8().constData());
            pendingEntity.m_parentId = entityObject["parentId"].toInteger();
            pendingEntity.m_components = entityObject["components"].toArray();
            pendingEntities.emplace(originalId, pendingEntity);
        }

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            for (const QJsonValue& componentValue : pendingEntity.m_components)
            {
                if (!componentValue.isObject())
                {
                    continue;
                }

                const QJsonObject componentObject = componentValue.toObject();
                const std::string componentName = componentObject["name"].toString().toStdString();
                const NexusEngine::ComponentDescriptor* descriptor = FindComponentDescriptor(componentName);
                if (descriptor && descriptor->m_addComponent)
                {
                    descriptor->m_addComponent(pendingEntity.m_entity);
                }
            }
        }

        for (auto& [originalId, pendingEntity] : pendingEntities)
        {
            (void)originalId;
            for (const QJsonValue& componentValue : pendingEntity.m_components)
            {
                if (!componentValue.isObject())
                {
                    continue;
                }

                const QJsonObject componentObject = componentValue.toObject();
                const std::string componentName = componentObject["name"].toString().toStdString();
                const NexusEngine::ComponentDescriptor* descriptor = FindComponentDescriptor(componentName);
                if (!descriptor || !descriptor->m_getProperties)
                {
                    continue;
                }

                const std::vector<NexusEngine::ComponentPropertyDescriptor> properties = descriptor->m_getProperties(pendingEntity.m_entity);
                const QJsonArray propertyArray = componentObject["properties"].toArray();
                for (const QJsonValue& propertyValue : propertyArray)
                {
                    if (!propertyValue.isObject())
                    {
                        continue;
                    }

                    const QJsonObject propertyObject = propertyValue.toObject();
                    const std::string propertyName = propertyObject["name"].toString().toStdString();
                    const std::string propertyText = propertyObject["value"].toString().toStdString();

                    for (const auto& property : properties)
                    {
                        if (property.m_name == propertyName && property.m_setValue)
                        {
                            property.m_setValue(pendingEntity.m_entity, propertyText);
                            break;
                        }
                    }
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

            auto parentIt = pendingEntities.find(pendingEntity.m_parentId);
            if (parentIt != pendingEntities.end())
            {
                pendingEntity.m_entity.child_of(parentIt->second.m_entity);
            }
        }

        return true;
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName)
    {
        return CreateEmptySceneFile(filePath, sceneName, QString{});
    }

    bool CreateEmptySceneFile(const QString& filePath, const QString& sceneName, const QString& assetGuid)
    {
        const QFileInfo fileInfo(filePath);
        if (!QDir().mkpath(fileInfo.absolutePath()))
        {
            return false;
        }

        QJsonObject sceneObject;
        sceneObject["guid"] = assetGuid.trimmed().isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces)
            : assetGuid.trimmed();
        sceneObject["name"] = sceneName;
        sceneObject["entities"] = QJsonArray{};

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        file.write(QJsonDocument(sceneObject).toJson(QJsonDocument::Indented));
        return file.error() == QFileDevice::NoError;
    }

    QString ReadSceneFileGuid(const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            return {};
        }

        return document.object()["guid"].toString().trimmed();
    }

    QString EnsureSceneFileGuid(const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject())
        {
            return {};
        }

        QJsonObject sceneObject = document.object();
        const QString existingGuid = sceneObject["guid"].toString().trimmed();
        if (!existingGuid.isEmpty())
        {
            return existingGuid;
        }

        const QString generatedGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        sceneObject["guid"] = generatedGuid;

        QFile outputFile(filePath);
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return {};
        }

        outputFile.write(QJsonDocument(sceneObject).toJson(QJsonDocument::Indented));
        return outputFile.error() == QFileDevice::NoError ? generatedGuid : QString{};
    }

    bool IsSceneFilePath(const QString& filePath)
    {
        return filePath.endsWith(GetSceneFileSuffix(), Qt::CaseInsensitive);
    }
} // namespace NexusEditor

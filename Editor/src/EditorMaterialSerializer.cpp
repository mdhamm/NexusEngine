#include "EditorMaterialSerializer.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace NexusEditor
{
    namespace
    {
        const QString& GetMaterialAssetFileSuffix()
        {
            static const QString s_materialFileSuffix = QStringLiteral(".nmat");
            return s_materialFileSuffix;
        }

        QJsonObject ToJsonObject(const MaterialAssetData& materialData)
        {
            QJsonObject materialObject;
            materialObject["name"] = materialData.m_name;
            materialObject["vertexShaderPath"] = materialData.m_vertexShaderPath;
            materialObject["pixelShaderPath"] = materialData.m_pixelShaderPath;
            materialObject["cullMode"] = materialData.m_cullMode;
            materialObject["isTransparent"] = materialData.m_isTransparent;
            materialObject["depthTestEnabled"] = materialData.m_depthTestEnabled;
            materialObject["depthWriteEnabled"] = materialData.m_depthWriteEnabled;
            return materialObject;
        }
    }

    bool CreateEmptyMaterialFile(const QString& filePath, const QString& materialName)
    {
        MaterialAssetData materialData;
        materialData.m_name = materialName;
        return SaveMaterialAssetFile(filePath, materialData);
    }

    bool LoadMaterialAssetFile(const QString& filePath, MaterialAssetData& materialData)
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

        const QJsonObject materialObject = document.object();
        materialData.m_name = materialObject["name"].toString(QFileInfo(filePath).completeBaseName()).trimmed();
        materialData.m_vertexShaderPath = materialObject["vertexShaderPath"].toString().trimmed();
        materialData.m_pixelShaderPath = materialObject["pixelShaderPath"].toString().trimmed();
        materialData.m_cullMode = materialObject["cullMode"].toString(QStringLiteral("None")).trimmed();
        materialData.m_isTransparent = materialObject["isTransparent"].toBool(false);
        materialData.m_depthTestEnabled = materialObject["depthTestEnabled"].toBool(true);
        materialData.m_depthWriteEnabled = materialObject["depthWriteEnabled"].toBool(true);
        return true;
    }

    bool SaveMaterialAssetFile(const QString& filePath, const MaterialAssetData& materialData)
    {
        const QFileInfo fileInfo(filePath);
        if (!QDir().mkpath(fileInfo.absolutePath()))
        {
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        file.write(QJsonDocument(ToJsonObject(materialData)).toJson(QJsonDocument::Indented));
        return file.error() == QFileDevice::NoError;
    }

    bool IsMaterialAssetFilePath(const QString& filePath)
    {
        return filePath.endsWith(GetMaterialAssetFileSuffix(), Qt::CaseInsensitive);
    }
} // namespace NexusEditor

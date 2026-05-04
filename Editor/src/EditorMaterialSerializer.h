#pragma once

#include <QString>

namespace NexusEditor
{
    struct MaterialAssetData
    {
        QString m_name;
        QString m_vertexShaderPath;
        QString m_pixelShaderPath;
        QString m_cullMode = QStringLiteral("None");
        bool m_isTransparent = false;
        bool m_depthTestEnabled = true;
        bool m_depthWriteEnabled = true;
    };

    /// <summary>
    /// Creates a material asset file on disk.
    /// </summary>
    /// <param name="filePath">Destination material asset path.</param>
    /// <param name="materialName">Material name stored in the asset.</param>
    /// <returns>True if the material asset was created; otherwise false.</returns>
    bool CreateEmptyMaterialFile(const QString& filePath, const QString& materialName);

    /// <summary>
    /// Loads a material asset from disk.
    /// </summary>
    /// <param name="filePath">Source material asset path.</param>
    /// <param name="materialData">Loaded material asset data.</param>
    /// <returns>True if the material asset was loaded; otherwise false.</returns>
    bool LoadMaterialAssetFile(const QString& filePath, MaterialAssetData& materialData);

    /// <summary>
    /// Saves a material asset to disk.
    /// </summary>
    /// <param name="filePath">Destination material asset path.</param>
    /// <param name="materialData">Material asset data to save.</param>
    /// <returns>True if the material asset was saved; otherwise false.</returns>
    bool SaveMaterialAssetFile(const QString& filePath, const MaterialAssetData& materialData);

    /// <summary>
    /// Returns whether a path points to a material asset file.
    /// </summary>
    /// <param name="filePath">Path to inspect.</param>
    /// <returns>True if the path uses the material asset extension; otherwise false.</returns>
    bool IsMaterialAssetFilePath(const QString& filePath);
} // namespace NexusEditor

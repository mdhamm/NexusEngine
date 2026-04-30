#pragma once

#include <QString>

#include <memory>

namespace NexusEditor
{
    struct AssetFileReferenceData;

    class AssetFileReference final
    {
    public:
        /// <summary>
        /// Creates a reference handle for the asset at the supplied path.
        /// </summary>
        /// <param name="path">Absolute or project-relative asset path.</param>
        /// <returns>A reference handle bound to the asset identity for that path.</returns>
        static AssetFileReference FromPath(const QString& path);

        /// <summary>
        /// Updates all live references when an asset path changes.
        /// </summary>
        /// <param name="oldPath">Previous absolute asset path.</param>
        /// <param name="newPath">New absolute asset path.</param>
        static void NotifyPathChanged(const QString& oldPath, const QString& newPath);

        /// <summary>
        /// Returns whether the asset reference has an assigned identity.
        /// </summary>
        /// <returns>True if the asset reference is empty; otherwise false.</returns>
        bool IsEmpty() const;

        /// <summary>
        /// Returns the referenced asset path.
        /// </summary>
        /// <returns>The referenced asset path.</returns>
        QString GetPath() const;

        /// <summary>
        /// Returns the stable guid for the referenced asset.
        /// </summary>
        /// <returns>The stable asset guid.</returns>
        QString GetGuid() const;

        /// <summary>
        /// Rebinds this handle to the asset identity stored at the supplied path.
        /// </summary>
        /// <param name="path">Absolute or project-relative asset path.</param>
        void BindToPath(const QString& path);

        /// <summary>
        /// Updates the referenced asset path while preserving its identity.
        /// </summary>
        /// <param name="path">New absolute or project-relative asset path.</param>
        void SetPath(const QString& path);

        /// <summary>
        /// Updates the referenced path when the asset is renamed or moved.
        /// </summary>
        /// <param name="oldPath">Previous absolute asset path.</param>
        /// <param name="newPath">New absolute asset path.</param>
        /// <returns>True if the reference path was updated; otherwise false.</returns>
        bool UpdatePath(const QString& oldPath, const QString& newPath);

        /// <summary>
        /// Resolves the referenced asset path inside a project root if the current path no longer exists.
        /// </summary>
        /// <param name="projectRootPath">Project root to search.</param>
        /// <returns>True if the reference resolves to a valid or writable location; otherwise false.</returns>
        bool ResolveWithinProject(const QString& projectRootPath) const;

        /// <summary>
        /// Constructs an empty asset reference.
        /// </summary>
        AssetFileReference() = default;

    private:
        explicit AssetFileReference(std::shared_ptr<AssetFileReferenceData> data);

        std::shared_ptr<AssetFileReferenceData> m_data;
    };
} // namespace NexusEditor

#pragma once

#include <memory>
#include <optional>
#include <QStringList>
#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <reflection/EntityReflection.h>

#include <cstdint>

class QString;

class QComboBox;
class QPushButton;
class QVBoxLayout;

namespace NexusEditor
{
    class EditorWindow;
    class PropertyWidgetSerializer;

    class PropertyWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the property inspector for the supplied scene view.
        /// </summary>
        /// <param name="editorWindow">Editor window that owns the engine instance.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit PropertyWidget(EditorWindow& editorWindow, QWidget* parent = nullptr);

        /// <summary>
        /// Sets the selected entity to inspect.
        /// </summary>
        /// <param name="entityId">Selected entity identifier.</param>
        void SetSelectedEntityId(std::uint64_t entityId);

        /// <summary>
        /// Sets the selected asset to inspect.
        /// </summary>
        /// <param name="assetPath">Selected asset path.</param>
        void SetSelectedAssetPath(const QString& assetPath);

        /// <summary>
        /// Refreshes the property inspector contents.
        /// </summary>
        void Refresh();

        /// <summary>
        /// Refreshes the property inspector only when the user is not actively interacting with it.
        /// </summary>
        void RefreshIfNotInteracting();

        /// <summary>
        /// Applies the currently pending asset-reference pick from a content drawer asset selection when possible.
        /// </summary>
        /// <param name="assetPath">Absolute or project-relative asset path selected in the content drawer.</param>
        /// <returns>True when the selection was consumed by an active asset-reference picker; otherwise false.</returns>
        bool TryAssignPickedAssetPath(const QString& assetPath);
        void NotifyEntityDeleted(std::uint64_t entityId);

        QStringList GetAssetPathsForType(NexusEngine::AssetType assetType) const;
        QString NormalizeAssetPath(const QString& assetPath) const;
        bool IsAcceptedAssetPath(const QString& assetPath, NexusEngine::AssetType assetType) const;
        void BeginAssetReferencePick(
            const std::string& componentName,
            const std::string& fieldName,
            NexusEngine::AssetType assetType,
            const QString& controlObjectName);
        bool IsAssetReferencePickActive(const QString& controlObjectName) const;
        void ClearAssetReferencePick();

    private:
        friend class PropertyWidgetSerializer;

        struct PendingAssetReferencePick
        {
            std::string m_componentName;
            std::string m_fieldName;
            NexusEngine::AssetType m_assetType = NexusEngine::AssetType::Material;
            QString m_controlObjectName;
        };

        QString CaptureStructureSignature() const;
        void SyncDisplayedValues();
        void RebuildAssetContents();
        void RebuildContents();
        void RebuildAddComponentOptions();
        void RemoveComponent(const std::string& componentName);
        bool IsInteracting() const;

        EditorWindow* m_editorWindow = nullptr;
        std::unique_ptr<PropertyWidgetSerializer> m_propertyWidgetSerializer;
        QComboBox* m_addComponentComboBox = nullptr;
        QPushButton* m_addComponentButton = nullptr;
        QVBoxLayout* m_contentLayout = nullptr;
        std::uint64_t m_selectedEntityId = 0;
        QString m_selectedAssetPath;
        QString m_lastStructureSignature;
        std::optional<PendingAssetReferencePick> m_pendingAssetReferencePick;
    };
} // namespace NexusEditor

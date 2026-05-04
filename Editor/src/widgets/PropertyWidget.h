#pragma once

#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <cstdint>

class QString;

class QComboBox;
class QPushButton;
class QVBoxLayout;

namespace NexusEditor
{
    class EditorWindow;

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

    private:
        QString CaptureStructureSignature() const;
        void SyncDisplayedValues();
        void RebuildAssetContents();
        void RebuildContents();
        void RebuildAddComponentOptions();
        bool IsInteracting() const;

        EditorWindow* m_editorWindow = nullptr;
        QComboBox* m_addComponentComboBox = nullptr;
        QPushButton* m_addComponentButton = nullptr;
        QVBoxLayout* m_contentLayout = nullptr;
        std::uint64_t m_selectedEntityId = 0;
        QString m_selectedAssetPath;
        QString m_lastStructureSignature;
    };
} // namespace NexusEditor

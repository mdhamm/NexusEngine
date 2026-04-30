#pragma once

#include <QWidget>

#include <cstdint>

class QComboBox;
class QVBoxLayout;

namespace NexusEditor
{
    class SceneViewWidget;

    class PropertyWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the property inspector for the supplied scene view.
        /// </summary>
        /// <param name="sceneView">Scene view that owns the engine instance.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit PropertyWidget(SceneViewWidget* sceneView, QWidget* parent = nullptr);

        /// <summary>
        /// Sets the selected entity to inspect.
        /// </summary>
        /// <param name="entityId">Selected entity identifier.</param>
        void SetSelectedEntityId(std::uint64_t entityId);

        /// <summary>
        /// Refreshes the property inspector contents.
        /// </summary>
        void Refresh();

    private:
        void RebuildContents();
        void RebuildAddComponentOptions();

        SceneViewWidget* m_sceneView = nullptr;
        QComboBox* m_addComponentComboBox = nullptr;
        QVBoxLayout* m_contentLayout = nullptr;
        std::uint64_t m_selectedEntityId = 0;
    };
} // namespace NexusEditor

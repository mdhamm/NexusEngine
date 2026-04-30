#pragma once

#include <QWidget>

class QTreeWidget;

namespace NexusEditor
{
    class SceneViewWidget;

    class SceneGraphWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the scene graph panel for the supplied scene view.
        /// </summary>
        /// <param name="sceneView">Scene view that owns the engine instance.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit SceneGraphWidget(SceneViewWidget* sceneView, QWidget* parent = nullptr);

        /// <summary>
        /// Refreshes the scene graph from the active scene.
        /// </summary>
        void Refresh();

    private:
        SceneViewWidget* m_sceneView = nullptr;
        QTreeWidget* m_treeWidget = nullptr;
    };
} // namespace NexusEditor

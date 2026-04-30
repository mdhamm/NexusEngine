#pragma once

#include <QMainWindow>

namespace NexusEditor
{
    class ContentDrawerWidget;
    class SceneGraphWidget;
    class SceneViewWidget;

    class EditorWindow final : public QMainWindow
    {
    public:
        /// <summary>
        /// Creates the main Nexus editor window.
        /// </summary>
        /// <param name="parent">Optional parent widget.</param>
        explicit EditorWindow(QWidget* parent = nullptr);

        /// <summary>
        /// Destroys the main editor window.
        /// </summary>
        ~EditorWindow() override = default;

    private:
        void BuildLayout();

        SceneViewWidget* m_sceneView = nullptr;
        SceneGraphWidget* m_sceneGraph = nullptr;
        ContentDrawerWidget* m_contentDrawer = nullptr;
    };
} // namespace NexusEditor

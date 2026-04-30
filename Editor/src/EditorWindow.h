#pragma once

#include "AssetFileReference.h"
#include "EditorProject.h"

#include <QMainWindow>

#include <QString>

namespace NexusEditor
{
    class ContentDrawerWidget;
    class PropertyWidget;
    class SceneGraphWidget;
    class SceneViewWidget;

    class EditorWindow final : public QMainWindow
    {
    public:
        /// <summary>
        /// Creates the main Nexus editor window.
        /// </summary>
        /// <param name="project">Project opened by the editor.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit EditorWindow(const EditorProject& project, QWidget* parent = nullptr);

        /// <summary>
        /// Destroys the main editor window.
        /// </summary>
        ~EditorWindow() override = default;

    private:
        void BuildMenus();
        void BuildLayout();
        void UpdateLoadedSceneFilePath(const QString& oldPath, const QString& newPath);
        bool ResolveSceneFilePath();
        void SaveScene();
        void SaveSceneAs();

        SceneViewWidget* m_sceneView = nullptr;
        SceneGraphWidget* m_sceneGraph = nullptr;
        PropertyWidget* m_propertyWidget = nullptr;
        ContentDrawerWidget* m_contentDrawer = nullptr;
        EditorProject m_project;
        AssetFileReference m_sceneFileReference;
    };
} // namespace NexusEditor

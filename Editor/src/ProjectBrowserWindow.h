#pragma once

#include "EditorProject.h"

#include <QMainWindow>

#include <functional>

class QListWidget;
class QPushButton;

namespace NexusEditor
{
    class ProjectBrowserWindow final : public QMainWindow
    {
    public:
        /// <summary>
        /// Creates the startup project browser window.
        /// </summary>
        /// <param name="parent">Optional parent widget.</param>
        explicit ProjectBrowserWindow(QWidget* parent = nullptr);

        /// <summary>
        /// Sets the callback invoked when a project is opened.
        /// </summary>
        /// <param name="callback">Callback receiving the selected project.</param>
        void SetProjectOpenedCallback(std::function<void(const EditorProject&)> callback);

    private:
        void RefreshProjectList();
        void OpenSelectedProject();
        void CreateProject();

        QListWidget* m_projectList = nullptr;
        QPushButton* m_openProjectButton = nullptr;
        std::function<void(const EditorProject&)> m_onProjectOpened;
    };
} // namespace NexusEditor

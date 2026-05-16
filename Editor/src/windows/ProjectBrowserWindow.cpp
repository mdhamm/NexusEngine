#include "ProjectBrowserWindow.h"

#include "EditorProjectRegistry.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace NexusEditor
{
    namespace
    {
        class CreateProjectDialog final : public QDialog
        {
        public:
            /// <summary>
            /// Creates the project creation dialog.
            /// </summary>
            /// <param name="parent">Optional parent widget.</param>
            explicit CreateProjectDialog(QWidget* parent = nullptr)
                : QDialog(parent)
            {
                setWindowTitle(QStringLiteral("Create Project"));

                auto* layout = new QVBoxLayout(this);

                auto* nameLabel = new QLabel(QStringLiteral("Project Name"), this);
                layout->addWidget(nameLabel);
                m_nameEdit = new QLineEdit(this);
                layout->addWidget(m_nameEdit);

                auto* locationLabel = new QLabel(QStringLiteral("Location"), this);
                layout->addWidget(locationLabel);

                auto* locationLayout = new QHBoxLayout();
                m_locationEdit = new QLineEdit(this);
                auto* browseButton = new QPushButton(QStringLiteral("Browse..."), this);
                locationLayout->addWidget(m_locationEdit, 1);
                locationLayout->addWidget(browseButton);
                layout->addLayout(locationLayout);

                auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
                layout->addWidget(buttonBox);

                connect(browseButton, &QPushButton::clicked, this, [this]()
                    {
                        const QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Project Location"), m_locationEdit->text());
                        if (!folder.isEmpty())
                        {
                            m_locationEdit->setText(folder);
                        }
                    });

                connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
                connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
            }

            /// <summary>
            /// Returns the entered project name.
            /// </summary>
            /// <returns>The entered project name.</returns>
            QString GetProjectName() const
            {
                return m_nameEdit->text().trimmed();
            }

            /// <summary>
            /// Returns the entered project location.
            /// </summary>
            /// <returns>The entered project location.</returns>
            QString GetLocationPath() const
            {
                return m_locationEdit->text().trimmed();
            }

        private:
            QLineEdit* m_nameEdit = nullptr;
            QLineEdit* m_locationEdit = nullptr;
        };
    }

    ProjectBrowserWindow::ProjectBrowserWindow(QWidget* parent)
        : QMainWindow(parent)
    {
        setWindowTitle(QStringLiteral("Nexus Projects"));
        resize(900, 600);

        auto* centralWidget = new QWidget(this);
        auto* layout = new QVBoxLayout(centralWidget);

        layout->addWidget(new QLabel(QStringLiteral("Recent Projects"), centralWidget));

        m_projectList = new QListWidget(centralWidget);
        layout->addWidget(m_projectList, 1);

        auto* buttonLayout = new QHBoxLayout();
        auto* createProjectButton = new QPushButton(QStringLiteral("Create New Project"), centralWidget);
        m_openProjectButton = new QPushButton(QStringLiteral("Open Project"), centralWidget);
        m_openProjectButton->setEnabled(false);
        buttonLayout->addWidget(createProjectButton);
        buttonLayout->addWidget(m_openProjectButton);
        layout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);

        connect(m_projectList, &QListWidget::currentRowChanged, this, [this](int row)
            {
                m_openProjectButton->setEnabled(row >= 0);
            });
        connect(m_projectList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { OpenSelectedProject(); });
        connect(m_openProjectButton, &QPushButton::clicked, this, [this]() { OpenSelectedProject(); });
        connect(createProjectButton, &QPushButton::clicked, this, [this]() { CreateProject(); });

        RefreshProjectList();
    }

    void ProjectBrowserWindow::SetProjectOpenedCallback(std::function<void(const EditorProject&)> callback)
    {
        m_onProjectOpened = std::move(callback);
    }

    void ProjectBrowserWindow::RefreshProjectList()
    {
        m_projectList->clear();

        const QVector<EditorProject> projects = EditorProjectRegistry::LoadRecentProjects();
        for (const EditorProject& project : projects)
        {
            auto* item = new QListWidgetItem(QStringLiteral("%1\n%2").arg(project.m_name, project.m_rootPath), m_projectList);
            item->setData(Qt::UserRole, project.m_name);
            item->setData(Qt::UserRole + 1, project.m_rootPath);
        }

        if (m_projectList->count() > 0)
        {
            m_projectList->setCurrentRow(0);
        }
    }

    void ProjectBrowserWindow::OpenSelectedProject()
    {
        QListWidgetItem* item = m_projectList->currentItem();
        if (!item || !m_onProjectOpened)
        {
            return;
        }

        EditorProject project;
        project.m_name = item->data(Qt::UserRole).toString();
        project.m_rootPath = item->data(Qt::UserRole + 1).toString();
        project.m_requiresInitialBuild = item->data(Qt::UserRole + 2).toBool();

        EditorProjectRegistry::AddRecentProject(project);
        m_onProjectOpened(project);
    }

    void ProjectBrowserWindow::CreateProject()
    {
        CreateProjectDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        EditorProject project;
        if (!EditorProjectRegistry::CreateProject(dialog.GetProjectName(), dialog.GetLocationPath(), project))
        {
            return;
        }

        RefreshProjectList();
        if (m_onProjectOpened)
        {
            m_onProjectOpened(project);
        }
    }
} // namespace NexusEditor

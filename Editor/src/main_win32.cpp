#include <QApplication>

#include <SDL.h>
#include <cstdio>
#include <memory>

#include "EditorWindow.h"
#include "ProjectBrowserWindow.h"

static void ReportError(const char* title, const char* detail)
{
    // Try an SDL message box if SDL is initialized, otherwise just print.
    if (SDL_WasInit(0) != 0)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, detail, nullptr);
    }
    std::fprintf(stderr, "%s: %s\n", title, detail ? detail : "(no detail)");
}

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        ReportError("SDL_Init failed", SDL_GetError());
        return 1;
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Nexus Editor"));
    app.setOrganizationName(QStringLiteral("NexusEngine"));

    NexusEditor::ProjectBrowserWindow projectBrowser;
    std::unique_ptr<NexusEditor::EditorWindow> editorWindow;

    projectBrowser.SetProjectOpenedCallback(
        [&](const NexusEditor::EditorProject& project)
        {
            editorWindow = std::make_unique<NexusEditor::EditorWindow>(project);
            editorWindow->show();
            projectBrowser.close();
        });

    projectBrowser.show();

    const int exitCode = app.exec();
    SDL_Quit();
    return exitCode;
}

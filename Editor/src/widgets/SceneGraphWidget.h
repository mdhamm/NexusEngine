#pragma once

#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <cstdint>
#include <functional>

class QTreeWidget;

namespace NexusEditor
{
    class EditorWindow;
    struct InspectedTarget;

    class SceneGraphWidget final : public QWidget
    {
    public:
        /// <summary>
        /// Creates the scene graph panel for the supplied scene view.
        /// </summary>
        /// <param name="editorWindow">Editor window that owns the engine instance.</param>
        /// <param name="parent">Optional parent widget.</param>
        explicit SceneGraphWidget(EditorWindow& editorWindow, QWidget* parent = nullptr);

        /// <summary>
        /// Refreshes the scene graph from the active scene.
        /// </summary>
        void Refresh();

        /// <summary>
        /// Clears the current tree selection without changing editor-owned inspector state.
        /// </summary>
        void ClearSelection();

        /// <summary>
        /// Sets the callback invoked when the selected entity changes.
        /// </summary>
        /// <param name="callback">Callback receiving the selected entity id.</param>
        void SetSelectionChangedCallback(std::function<void(std::uint64_t)> callback);

    private:
        void HandleInspectedTargetChanged(const InspectedTarget& inspectedTarget);
        void DeleteSelectedEntity();

        EditorWindow* m_editorWindow = nullptr;
        QTreeWidget* m_treeWidget = nullptr;
        std::function<void(std::uint64_t)> m_onSelectionChanged;
        std::uint64_t m_selectedEntityId = 0;
    };
} // namespace NexusEditor

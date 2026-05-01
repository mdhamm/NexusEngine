#pragma once

#include <QWidget>

#ifdef emit
#undef emit
#endif

#include <cstdint>

class QComboBox;
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
        /// Refreshes the property inspector contents.
        /// </summary>
        void Refresh();

    private:
        void RebuildContents();
        void RebuildAddComponentOptions();

        EditorWindow* m_editorWindow = nullptr;
        QComboBox* m_addComponentComboBox = nullptr;
        QVBoxLayout* m_contentLayout = nullptr;
        std::uint64_t m_selectedEntityId = 0;
    };
} // namespace NexusEditor

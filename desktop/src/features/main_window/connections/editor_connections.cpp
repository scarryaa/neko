#include "editor_connections.h"

EditorConnections::EditorConnections(const WorkspaceUiHandles &uiHandles,
                                     EditorController *editorController,
                                     WorkspaceCoordinator *workspaceCoordinator,
                                     QObject *parent)
    : QObject(parent) {
  // GutterWidget <-> EditorWidget
  connect(uiHandles.gutterWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget, &EditorWidget::fontSizeChanged,
          uiHandles.gutterWidget, &GutterWidget::onEditorFontSizeChanged);

  // EditorController -> StatusBarWidget
  connect(editorController, &EditorController::cursorChanged,
          uiHandles.statusBarWidget, &StatusBarWidget::onCursorPositionChanged);

  // EditorController -> MainWindow
  connect(editorController, &EditorController::bufferChanged,
          workspaceCoordinator, &WorkspaceCoordinator::bufferChanged);
}

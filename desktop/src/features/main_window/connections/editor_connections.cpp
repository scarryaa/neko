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

  // EditorController -> EditorWidget
  connect(editorController, &EditorController::bufferChanged,
          uiHandles.editorWidget, &EditorWidget::onBufferChanged);
  connect(editorController, &EditorController::cursorChanged,
          uiHandles.editorWidget, &EditorWidget::onCursorChanged);
  connect(editorController, &EditorController::selectionChanged,
          uiHandles.editorWidget, &EditorWidget::onSelectionChanged);
  connect(editorController, &EditorController::viewportChanged,
          uiHandles.editorWidget, &EditorWidget::onViewportChanged);

  // EditorController -> GutterWidget
  connect(editorController, &EditorController::lineCountChanged,
          uiHandles.gutterWidget, &GutterWidget::onEditorLineCountChanged);
  connect(editorController, &EditorController::bufferChanged,
          uiHandles.gutterWidget, &GutterWidget::onBufferChanged);
  connect(editorController, &EditorController::cursorChanged,
          uiHandles.gutterWidget, &GutterWidget::onCursorChanged);
  connect(editorController, &EditorController::selectionChanged,
          uiHandles.gutterWidget, &GutterWidget::onSelectionChanged);
  connect(editorController, &EditorController::viewportChanged,
          uiHandles.gutterWidget, &GutterWidget::onViewportChanged);
}

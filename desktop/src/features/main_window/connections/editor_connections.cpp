#include "editor_connections.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include <QScrollBar>

EditorConnections::EditorConnections(const EditorConnectionsProps &props,
                                     QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *workspaceCoordinator = props.workspaceCoordinator;
  auto *editorController = props.editorController;

  // GutterWidget <-> EditorWidget
  connect(uiHandles.gutterWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget, &EditorWidget::fontSizeChangedByUser,
          uiHandles.gutterWidget, &GutterWidget::onEditorFontSizeChanged);

  // EditorController -> StatusBarWidget
  connect(editorController, &EditorController::cursorChanged,
          uiHandles.statusBarWidget, &StatusBarWidget::onCursorPositionChanged);

  // EditorController -> WorkspaceCoordinator
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

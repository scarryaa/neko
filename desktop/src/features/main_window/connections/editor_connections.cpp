#include "editor_connections.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include <QScrollBar>

EditorConnections::EditorConnections(const EditorConnectionsProps &props,
                                     QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *workspaceCoordinator = props.workspaceCoordinator;
  auto *editorBridge = props.editorBridge;

  // GutterWidget <-> EditorWidget
  connect(uiHandles.gutterWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget->verticalScrollBar(),
          &QScrollBar::valueChanged,
          uiHandles.gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(uiHandles.editorWidget, &EditorWidget::fontSizeChangedByUser,
          uiHandles.gutterWidget, &GutterWidget::onEditorFontSizeChanged);

  // EditorBridge -> StatusBarWidget
  connect(editorBridge, &EditorBridge::cursorChanged, uiHandles.statusBarWidget,
          &StatusBarWidget::onCursorPositionChanged);

  // EditorBridge -> WorkspaceCoordinator
  connect(editorBridge, &EditorBridge::bufferChanged, workspaceCoordinator,
          &WorkspaceCoordinator::bufferChanged);

  // EditorBridge -> EditorWidget
  connect(editorBridge, &EditorBridge::bufferChanged, uiHandles.editorWidget,
          &EditorWidget::onBufferChanged);
  connect(editorBridge, &EditorBridge::cursorChanged, uiHandles.editorWidget,
          &EditorWidget::onCursorChanged);
  connect(editorBridge, &EditorBridge::selectionChanged, uiHandles.editorWidget,
          &EditorWidget::onSelectionChanged);
  connect(editorBridge, &EditorBridge::viewportChanged, uiHandles.editorWidget,
          &EditorWidget::onViewportChanged);

  // EditorBridge -> GutterWidget
  connect(editorBridge, &EditorBridge::lineCountChanged, uiHandles.gutterWidget,
          &GutterWidget::onEditorLineCountChanged);
  connect(editorBridge, &EditorBridge::bufferChanged, uiHandles.gutterWidget,
          &GutterWidget::onBufferChanged);
  connect(editorBridge, &EditorBridge::cursorChanged, uiHandles.gutterWidget,
          &GutterWidget::onCursorChanged);
  connect(editorBridge, &EditorBridge::selectionChanged, uiHandles.gutterWidget,
          &GutterWidget::onSelectionChanged);
  connect(editorBridge, &EditorBridge::viewportChanged, uiHandles.gutterWidget,
          &GutterWidget::onViewportChanged);
}

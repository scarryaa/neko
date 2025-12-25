#include "workspace_connections.h"
#include "features/editor/editor_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/tabs/tab_bar_widget.h"

WorkspaceConnections::WorkspaceConnections(
    const WorkspaceConnectionsProps &props, QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *workspaceCoordinator = props.workspaceCoordinator;

  // WorkspaceCoordinator -> TabBarWidget
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabCloseRequested,
          workspaceCoordinator, &WorkspaceCoordinator::closeTab);
  connect(uiHandles.tabBarWidget, &TabBarWidget::currentChanged,
          workspaceCoordinator, &WorkspaceCoordinator::tabChanged);
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabUnpinRequested,
          workspaceCoordinator, &WorkspaceCoordinator::tabUnpinned);
  connect(uiHandles.tabBarWidget, &TabBarWidget::newTabRequested,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);

  // EditorWidget -> WorkspaceCoordinator
  connect(uiHandles.editorWidget, &EditorWidget::newTabRequested,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);
}

#include "workspace_connections.h"

WorkspaceConnections::WorkspaceConnections(
    const WorkspaceUiHandles &uiHandles,
    WorkspaceCoordinator *workspaceCoordinator, QObject *parent)
    : QObject(parent) {
  // WorkspaceCoordinator -> TabBarWidget
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabCloseRequested,
          workspaceCoordinator, &WorkspaceCoordinator::closeTab);
  connect(uiHandles.tabBarWidget, &TabBarWidget::currentChanged,
          workspaceCoordinator, &WorkspaceCoordinator::tabChanged);
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabPinnedChanged,
          workspaceCoordinator, &WorkspaceCoordinator::tabChanged);
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabUnpinRequested,
          workspaceCoordinator, &WorkspaceCoordinator::tabUnpinned);
  connect(uiHandles.tabBarWidget, &TabBarWidget::newTabRequested,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);

  // EditorWidget -> WorkspaceCoordinator
  connect(uiHandles.editorWidget, &EditorWidget::newTabRequested,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);
}

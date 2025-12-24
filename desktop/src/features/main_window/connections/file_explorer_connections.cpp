#include "file_explorer_connections.h"

FileExplorerConnections::FileExplorerConnections(
    const WorkspaceUiHandles &uiHandles, QObject *parent)
    : QObject(parent) {
  // FileExplorerWidget -> TitleBarWidget
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::directorySelected,
          uiHandles.titleBarWidget, &TitleBarWidget::onDirChanged);

  // TitleBarWidget -> FileExplorerWidget
  connect(uiHandles.titleBarWidget,
          &TitleBarWidget::directorySelectionButtonPressed,
          uiHandles.fileExplorerWidget,
          &FileExplorerWidget::directorySelectionRequested);
}

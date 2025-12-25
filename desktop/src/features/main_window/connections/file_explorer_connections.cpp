#include "file_explorer_connections.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/title_bar/title_bar_widget.h"

FileExplorerConnections::FileExplorerConnections(
    const WorkspaceUiHandles &uiHandles, QObject *parent)
    : QObject(parent) {
  // FileExplorerWidget -> TitleBarWidget
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::directorySelected,
          uiHandles.titleBarWidget, &TitleBarWidget::directoryChanged);

  // TitleBarWidget -> FileExplorerWidget
  connect(uiHandles.titleBarWidget,
          &TitleBarWidget::directorySelectionButtonPressed,
          uiHandles.fileExplorerWidget,
          &FileExplorerWidget::directorySelectionRequested);
}

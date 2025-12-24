#include "main_window_connections.h"

MainWindowConnections::MainWindowConnections(
    const WorkspaceUiHandles &uiHandles,
    WorkspaceCoordinator *workspaceCoordinator, ThemeProvider *themeProvider,
    QObject *parent)
    : QObject(parent) {
  // NewTabButton -> WorkspaceCoordinator
  connect(uiHandles.newTabButton, &QPushButton::clicked, workspaceCoordinator,
          &WorkspaceCoordinator::newTab);

  // EmptyStateNewTabButton -> WorkspaceCoordinator
  connect(uiHandles.emptyStateNewTabButton, &QPushButton::clicked,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);

  // WorkspaceCoordinator -> MainWindow
  connect(workspaceCoordinator, &WorkspaceCoordinator::themeChanged,
          themeProvider, &ThemeProvider::reload);

  // FileExplorerWidget -> MainWindow
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::fileSelected,
          workspaceCoordinator, &WorkspaceCoordinator::fileSelected);

  // StatusBarWidget -> MainWindow
  connect(uiHandles.statusBarWidget, &StatusBarWidget::fileExplorerToggled,
          workspaceCoordinator, &WorkspaceCoordinator::fileExplorerToggled);
  connect(workspaceCoordinator,
          &WorkspaceCoordinator::onFileExplorerToggledViaShortcut,
          uiHandles.statusBarWidget,
          &StatusBarWidget::onFileExplorerToggledExternally);
  connect(uiHandles.statusBarWidget, &StatusBarWidget::cursorPositionClicked,
          workspaceCoordinator, &WorkspaceCoordinator::cursorPositionClicked);

  // CommandPalette -> MainWindow
  connect(uiHandles.commandPaletteWidget,
          &CommandPaletteWidget::goToPositionRequested, workspaceCoordinator,
          &WorkspaceCoordinator::commandPaletteGoToPosition);
  connect(uiHandles.commandPaletteWidget,
          &CommandPaletteWidget::commandRequested, workspaceCoordinator,
          &WorkspaceCoordinator::commandPaletteCommand);
}

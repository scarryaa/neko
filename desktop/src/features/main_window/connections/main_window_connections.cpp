#include "main_window_connections.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "theme/theme_provider.h"
#include <QPushButton>

MainWindowConnections::MainWindowConnections(
    const MainWindowConnectionsProps &props, QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *workspaceCoordinator = props.workspaceCoordinator;
  auto *themeProvider = props.themeProvider;

  // NewTabButton -> WorkspaceCoordinator
  connect(uiHandles.newTabButton, &QPushButton::clicked, workspaceCoordinator,
          &WorkspaceCoordinator::newTab);

  // EmptyStateNewTabButton -> WorkspaceCoordinator
  connect(uiHandles.emptyStateNewTabButton, &QPushButton::clicked,
          workspaceCoordinator, &WorkspaceCoordinator::newTab);

  // WorkspaceCoordinator -> ThemeProvider
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

  // WorkspaceCoordinator -> FileExplorerWidget
  connect(
      workspaceCoordinator, &WorkspaceCoordinator::tabRevealedInFileExplorer,
      uiHandles.fileExplorerWidget, &FileExplorerWidget::itemRevealRequested);

  // CommandPalette -> MainWindow
  connect(uiHandles.commandPaletteWidget,
          &CommandPaletteWidget::goToPositionRequested, workspaceCoordinator,
          &WorkspaceCoordinator::commandPaletteGoToPosition);
  connect(uiHandles.commandPaletteWidget,
          &CommandPaletteWidget::commandRequested, workspaceCoordinator,
          &WorkspaceCoordinator::commandPaletteCommand);
}

#include "file_explorer_connections.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_config_service.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/title_bar/title_bar_widget.h"

FileExplorerConnections::FileExplorerConnections(
    const FileExplorerConnectionsProps &props, QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *appConfigService = props.appConfigService;

  // FileExplorerWidget -> TitleBarWidget
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::directorySelected,
          uiHandles.titleBarWidget, &TitleBarWidget::directoryChanged);

  // TitleBarWidget -> FileExplorerWidget
  connect(uiHandles.titleBarWidget,
          &TitleBarWidget::directorySelectionButtonPressed,
          uiHandles.fileExplorerWidget,
          &FileExplorerWidget::directorySelectionRequested);

  // FileExplorerConnections -> FileExplorerWidget (load saved dir)
  connect(this, &FileExplorerConnections::savedDirectoryLoaded,
          uiHandles.fileExplorerWidget,
          &FileExplorerWidget::loadSavedDirectory);

  // FileExplorerWidget -> AppConfigService (update font size)
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::fontSizeChanged,
          appConfigService, [appConfigService](double size) {
            appConfigService->setFileExplorerFontSize(static_cast<int>(size));
          });

  // FileExplorerWidget -> AppConfigService (persist directory)
  connect(uiHandles.fileExplorerWidget,
          &FileExplorerWidget::directoryPersistRequested, appConfigService,
          [appConfigService](const std::string &path) {
            appConfigService->setFileExplorerDirectory(path);
          });

  // FileExplorerWidget -> load saved directory on startup
  const auto snapshot = appConfigService->getSnapshot();
  if (snapshot.file_explorer_directory_present &&
      !snapshot.file_explorer_directory.empty()) {
    emit savedDirectoryLoaded(
        QString::fromUtf8(snapshot.file_explorer_directory).toStdString());
  }
}

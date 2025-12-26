#include "file_explorer_connections.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/title_bar/title_bar_widget.h"
#include "utils/ui_utils.h"

FileExplorerConnections::FileExplorerConnections(
    const FileExplorerConnectionsProps &props, QObject *parent)
    : QObject(parent) {
  auto uiHandles = props.uiHandles;
  auto *configManager = props.configManager;

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

  // TODO(scarlet): Move to dedicated ConfigController?
  // FileExplorerWidget -> update font size
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::fontSizeChanged,
          this, [this, configManager](double size) {
            UiUtils::setFontSize(*configManager, neko::FontType::FileExplorer,
                                 size);
          });

  // TODO(scarlet): Move to dedicated ConfigController?
  connect(uiHandles.fileExplorerWidget,
          &FileExplorerWidget::directoryPersistRequested, this,
          [this, configManager](const std::string &path) {
            auto snapshot = configManager->get_config_snapshot();
            snapshot.file_explorer_directory_present = true;
            snapshot.file_explorer_directory = path;

            configManager->apply_config_snapshot(snapshot);
          });

  // FileExplorerWidget -> load saved directory
  auto snapshot = configManager->get_config_snapshot();
  if (snapshot.file_explorer_directory_present &&
      !snapshot.file_explorer_directory.empty()) {
    emit savedDirectoryLoaded(snapshot.file_explorer_directory.c_str());
  }
}

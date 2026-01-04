#include "file_explorer_flows.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/services/file_io_service.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

FileExplorerFlows::FileExplorerFlows(const FileExplorerFlowsProps &props)
    : appBridge(props.appBridge), uiHandles(props.uiHandles) {}

bool FileExplorerFlows::handleFileExplorerCommand(
    const std::string &commandId, const neko::FileExplorerContextFfi &ctx) {
  if (commandId.empty()) {
    return false;
  }

  const auto itemPath = QString::fromUtf8(ctx.item_path);
  // TODO(scarlet): Avoid bypassing FileTreeBridge.
  auto fileTreeController = appBridge->getFileTreeController();
  auto parentItemPath = QString::fromUtf8(
      fileTreeController->get_path_of_parent(itemPath.toStdString()));
  bool shouldRedraw = false;

  // Handle Qt-side special cases.
  // TODO(scarlet): Move these to Rust eventually.
  // TODO(scarlet): Handle result.
  if (commandId == "fileExplorer.cut") {
    FileIoService::cut(itemPath);
  } else if (commandId == "fileExplorer.copy") {
    FileIoService::copy(itemPath);
  } else if (commandId == "fileExplorer.duplicate") {
    shouldRedraw = true;
    auto result = FileIoService::duplicate(itemPath);

    if (result.success) {
      const auto newItemPath = result.newPath;
      // TODO(scarlet): Avoid bypassing FileTreeBridge.
      fileTreeController->set_current_path(newItemPath.toStdString());
    }
  } else if (commandId == "fileExplorer.paste") {
    shouldRedraw = true;
    FileIoService::paste(itemPath);
    // TODO(scarlet): Select the new pasted item.
  } else if (commandId == "fileExplorer.copyPath") {
    // Path is likely already absolute, but convert it just in case.
    QFileInfo fileInfo(itemPath);

    const auto absolutePath = fileInfo.absoluteFilePath();
    QApplication::clipboard()->setText(absolutePath);
  } else if (commandId == "fileExplorer.copyRelativePath") {
    const QString workspaceRootPath =
        QString::fromUtf8(appBridge->getFileTreeController()->get_root_path());
    QDir rootDir(workspaceRootPath);

    const QString relativePath = rootDir.relativeFilePath(itemPath);
    QApplication::clipboard()->setText(relativePath);
  }

  if (shouldRedraw) {
    // TODO(scarlet): Convert this to a signal.
    uiHandles.fileExplorerWidget->redraw();
    // TODO(scarlet): Avoid bypassing FileTreeBridge.
    fileTreeController->refresh_dir(parentItemPath.toStdString());
  }

  // TODO(scarlet): Get new item name and rename item name args from a dialog as
  // needed.
  const auto commandResult =
      appBridge->runCommand<neko::FileExplorerCommandResultFfi>(
          CommandType::FileExplorer, commandId, ctx, "", "");

  for (const auto &intent : commandResult.intents) {
    switch (intent.kind) {
    case neko::FileExplorerUiIntentKindFfi::DirectoryRefreshed:
      // TODO(scarlet): Convert this to a signal.
      uiHandles.fileExplorerWidget->redraw();
      break;
    }
  }

  return true;
}

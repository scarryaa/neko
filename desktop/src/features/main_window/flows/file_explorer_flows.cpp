#include "file_explorer_flows.h"
#include "features/file_explorer/file_explorer_widget.h"
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

  // Handle Qt-side special cases.
  // TODO(scarlet): Move these to Rust eventually.
  if (commandId == "fileExplorer.cut") {

  } else if (commandId == "fileExplorer.copy") {

  } else if (commandId == "fileExplorer.duplicate") {

  } else if (commandId == "fileExplorer.paste") {

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

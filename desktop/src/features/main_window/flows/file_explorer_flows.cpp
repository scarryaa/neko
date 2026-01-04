#include "file_explorer_flows.h"
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
  appBridge->runCommand<neko::FileExplorerContextFfi>(CommandType::FileExplorer,
                                                      commandId, ctx, "", "");

  // // TODO(scarlet): Avoid matching on hardcoded command ids
  // bool succeeded = false;
  // if (commandId == "tab.close") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::Single, tabId, forceClose);
  // } else if (commandId == "tab.closeOthers") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::Others, tabId, forceClose);
  // } else if (commandId == "tab.closeLeft") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::Left, tabId, forceClose);
  // } else if (commandId == "tab.closeRight") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::Right, tabId, forceClose);
  // } else if (commandId == "tab.closeAll") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::All, tabId, forceClose);
  // } else if (commandId == "tab.closeClean") {
  //   succeeded =
  //       closeTabs(neko::CloseTabOperationTypeFfi::Clean, tabId, forceClose);
  // } else if (commandId == "tab.copyPath") {
  //   succeeded = copyTabPath(tabId);
  // } else if (commandId == "tab.reveal") {
  //   succeeded = revealTab(ctx);
  // } else if (commandId == "tab.pin") {
  //   succeeded = tabTogglePin(tabId, ctx.is_pinned);
  // }

  return true;
}

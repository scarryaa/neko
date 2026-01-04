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

  // Get the parent path ahead of time, in case the operation is a
  // delete/rename, after which the original item path would not exist anymore.
  auto parentItemPath = QString::fromUtf8(
      fileTreeController->get_path_of_parent(itemPath.toStdString()));

  bool shouldRedraw = false;
  bool success = false;

  // Handle Qt-side special cases.
  // TODO(scarlet): Move these to Rust eventually.
  // TODO(scarlet): Avoid matching on hardcoded command ids?
  if (commandId == "fileExplorer.cut") {
    handleCut(itemPath);
  } else if (commandId == "fileExplorer.copy") {
    handleCopy(itemPath);
  } else if (commandId == "fileExplorer.duplicate") {
    // TODO(scarlet): Fix duplicated item not showing up in the tree.
    shouldRedraw = true;
    success = handleDuplicate(itemPath, parentItemPath, &*fileTreeController);
  } else if (commandId == "fileExplorer.paste") {
    shouldRedraw = true;
    success = handlePaste(itemPath, parentItemPath, &*fileTreeController);
  } else if (commandId == "fileExplorer.copyPath") {
    handleCopyPath(itemPath);
  } else if (commandId == "fileExplorer.copyRelativePath") {
    handleCopyRelativePath(itemPath, &*fileTreeController);
  } else {
    // Set success to true here if none of the command ids were a match, so we
    // can let Rust handle it.
    success = true;
  }

  if (!success) {
    return false;
  }

  bool itemIsExpanded = false;
  const auto snapshot = fileTreeController->get_tree_snapshot();
  for (const auto &node : snapshot.nodes) {
    if (node.path == itemPath && node.is_dir) {
      itemIsExpanded = node.is_expanded;
    }
  }

  if (commandId == "fileExplorer.delete") {
    // TODO(scarlet): Show a confirmation prompt (unless shift is held).
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

  // TODO(scarlet): Move these to a fn?
  // When creating or modifying an item, unconditionally re-expand the directory
  // containing the item (except for rename operations, we only re-expand if it
  // was expanded to begin with).
  if (commandId == "fileExplorer.delete") {
    // Prevent expanding the root's parent (which is not part of the tree -- and
    // the root would be deleted anyway if the deletion target was the root
    // directory).
    // TODO(scarlet): Handle case where root folder is deleted.
    const QString workspaceRootPath =
        QString::fromUtf8(fileTreeController->get_root_path());

    if (itemPath != workspaceRootPath) {
      fileTreeController->set_expanded(parentItemPath.toStdString());
    }
  }

  if (commandId == "fileExplorer.newFile" ||
      commandId == "fileExplorer.newFolder" ||
      commandId == "fileExplorer.duplicate") {
    fileTreeController->set_expanded(parentItemPath.toStdString());
  }

  if (commandId == "fileExplorer.rename") {
    if (itemIsExpanded) {
      fileTreeController->set_expanded(parentItemPath.toStdString());
    }
  }

  if (shouldRedraw) {
    // TODO(scarlet): Convert this to a signal.
    uiHandles.fileExplorerWidget->redraw();
  }

  return true;
}

void FileExplorerFlows::handleCut(const QString &itemPath) {
  FileIoService::cut(itemPath);
}

void FileExplorerFlows::handleCopy(const QString &itemPath) {
  FileIoService::copy(itemPath);
}

bool FileExplorerFlows::handleDuplicate(
    const QString &itemPath, const QString &parentItemPath,
    neko::FileTreeController *fileTreeController) {
  const auto result = FileIoService::duplicate(itemPath);

  if (result.success) {
    const auto newItemPath = result.newPath;
    // TODO(scarlet): Avoid bypassing FileTreeBridge.
    fileTreeController->refresh_dir(parentItemPath.toStdString());
    fileTreeController->set_current_path(newItemPath.toStdString());
  }

  return result.success;
}

// TODO(scarlet): Clean this up.
bool FileExplorerFlows::handlePaste(
    const QString &itemPath, const QString &parentItemPath,
    neko::FileTreeController *fileTreeController) {
  const auto result = FileIoService::paste(itemPath);
  bool destinationIsDirectory = QFileInfo(itemPath).isDir();

  if (result.success) {
    if (result.wasCutOperation) {
      // On a cut/paste, we need to refresh both the destination
      // directory and the source directory.
      if (destinationIsDirectory) {
        fileTreeController->refresh_dir(itemPath.toStdString());

        // TODO(scarlet): Handle multiple selected items eventually.
        QString originalPath = result.items.first().originalPath;
        bool sourceIsDirectory = QFileInfo(originalPath).isDir();

        if (sourceIsDirectory) {
          fileTreeController->refresh_dir(originalPath.toStdString());
          fileTreeController->set_expanded(originalPath.toStdString());
        } else {
          const auto originalParentPath =
              QString::fromUtf8(fileTreeController->get_path_of_parent(
                  originalPath.toStdString()));
          fileTreeController->refresh_dir(originalParentPath.toStdString());
          fileTreeController->set_expanded(originalParentPath.toStdString());
        }
      } else {
        fileTreeController->refresh_dir(parentItemPath.toStdString());

        QString originalPath = result.items.first().originalPath;
        bool sourceIsDirectory = QFileInfo(originalPath).isDir();

        if (sourceIsDirectory) {
          fileTreeController->refresh_dir(originalPath.toStdString());
          fileTreeController->set_expanded(originalPath.toStdString());
        } else {
          const auto originalParentPath =
              QString::fromUtf8(fileTreeController->get_path_of_parent(
                  originalPath.toStdString()));
          fileTreeController->refresh_dir(originalParentPath.toStdString());
          fileTreeController->set_expanded(originalParentPath.toStdString());
        }
      }
    } else {
      // Otherwise, just refresh the destination directory.
      if (destinationIsDirectory) {
        fileTreeController->refresh_dir(itemPath.toStdString());
      } else {
        fileTreeController->refresh_dir(parentItemPath.toStdString());
      }
    }
  } else {
    // If the paste failed, exit.
    return false;
  }

  // If the destination is a directory, refresh it. Otherwise, refresh the
  // parent directory.
  // TODO(scarlet): Avoid bypassing FileTreeBridge for fileTreeController.
  if (destinationIsDirectory) {
    fileTreeController->refresh_dir(itemPath.toStdString());
  } else {
    fileTreeController->refresh_dir(parentItemPath.toStdString());
  }

  // Avoid collapsing the directory on reload.
  fileTreeController->set_expanded(destinationIsDirectory
                                       ? itemPath.toStdString()
                                       : parentItemPath.toStdString());

  QString newItemPath = result.items.first().newPath;
  fileTreeController->set_current_path(newItemPath.toStdString());

  return true;
}

void FileExplorerFlows::handleCopyPath(const QString &itemPath) {
  // Path is likely already absolute, but convert it just in case.
  QFileInfo fileInfo(itemPath);

  const auto absolutePath = fileInfo.absoluteFilePath();
  QApplication::clipboard()->setText(absolutePath);
}

void FileExplorerFlows::handleCopyRelativePath(
    const QString &itemPath, neko::FileTreeController *fileTreeController) {
  const QString workspaceRootPath =
      QString::fromUtf8(fileTreeController->get_root_path());
  QDir rootDir(workspaceRootPath);

  const QString relativePath = rootDir.relativeFilePath(itemPath);
  QApplication::clipboard()->setText(relativePath);
}

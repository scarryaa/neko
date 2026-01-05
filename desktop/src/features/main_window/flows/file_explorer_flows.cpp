#include "file_explorer_flows.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/services/file_io_service.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

FileExplorerFlows::FileExplorerFlows(const FileExplorerFlowsProps &props)
    : appBridge(props.appBridge), uiHandles(props.uiHandles),
      fileTreeBridge(props.fileTreeBridge) {}

// TODO(scarlet): Handle open tab updates when, e.g., a file that is currently
// open in a tab is deleted/renamed/moved/etc.
FileExplorerFlows::FileExplorerFlowsCommandResult
FileExplorerFlows::handleFileExplorerCommand(
    const std::string &commandId, const neko::FileExplorerContextFfi &ctx,
    bool bypassDeleteConfirmation) {
  FileExplorerFlowsCommandResult result{
      .success = false,
      .shouldRedraw = false,
      .intentKinds = std::vector<neko::FileExplorerUiIntentKindFfi>()};
  if (commandId.empty()) {
    return result;
  }

  const auto itemPath = QString::fromUtf8(ctx.item_path);

  // Get the parent path ahead of time, in case the operation is a
  // delete/rename, after which the original item path would not exist anymore.
  auto parentItemPath = QString::fromUtf8(
      fileTreeBridge->getParentNodePath(itemPath.toStdString()));

  // Handle Qt-side special cases.
  // TODO(scarlet): Move these to Rust eventually.
  // TODO(scarlet): Avoid matching on hardcoded command ids?
  if (commandId == "fileExplorer.cut") {
    handleCut(itemPath);
  } else if (commandId == "fileExplorer.copy") {
    handleCopy(itemPath);
  } else if (commandId == "fileExplorer.duplicate") {
    result.shouldRedraw = true;
    result.success = handleDuplicate(itemPath, parentItemPath);
  } else if (commandId == "fileExplorer.paste") {
    result.shouldRedraw = true;
    result.success = handlePaste(itemPath, parentItemPath);
  } else if (commandId == "fileExplorer.copyPath") {
    handleCopyPath(itemPath);
  } else if (commandId == "fileExplorer.copyRelativePath") {
    handleCopyRelativePath(itemPath);
  } else {
    // Set success to true here if none of the command ids were a match, so we
    // can let Rust handle it.
    result.success = true;
  }

  if (!result.success) {
    // Reset `shouldRedraw` if the command failed.
    result.shouldRedraw = false;
    return result;
  }

  bool itemIsExpanded = false;
  const auto snapshot = fileTreeBridge->getTreeSnapshot();
  for (const auto &node : snapshot.nodes) {
    if ((node.path == itemPath && node.is_dir) ||
        (node.path == parentItemPath && node.is_dir)) {
      // Check if the node is expanded (if it is a directory). If not a
      // directory, check if its parent is expanded.
      itemIsExpanded = node.is_expanded;
    }
  }

  bool isNewFileCommand = commandId == "fileExplorer.newFile";
  bool isNewDirectoryCommand = commandId == "fileExplorer.newFolder";
  bool isRenameCommand = commandId == "fileExplorer.rename";
  QFileInfo itemFileInfo(itemPath);
  bool itemIsDirectory = itemFileInfo.isDir();
  QString newItemName;

  // Open a delete confirmation dialog unless bypassDeleteConfirmation is true.
  if (commandId == "fileExplorer.delete" && !bypassDeleteConfirmation) {
    using Type = DialogService::DeleteItemType;
    using Decision = DialogService::DeleteDecision;
    Type type = itemIsDirectory ? Type::Directory : Type::File;

    const auto deleteResult = DialogService::openDeleteConfirmationDialog(
        itemFileInfo.fileName(), type, uiHandles.window);
    if (deleteResult == Decision::Cancel) {
      result.shouldRedraw = false;
      return result;
    }
  }

  if (isNewFileCommand || isNewDirectoryCommand || isRenameCommand) {
    using Type = DialogService::OperationType;

    QFileInfo srcInfo(itemPath);
    QString originalItemName = srcInfo.fileName();
    Type type;

    if (isNewFileCommand) {
      type = Type::NewFile;
    } else if (isNewDirectoryCommand) {
      type = Type::NewDirectory;
    } else if (isRenameCommand) {
      type = itemIsDirectory ? Type::RenameDirectory : Type::RenameFile;
    }

    newItemName = DialogService::openItemNameDialog(
        uiHandles.window, type, isRenameCommand ? originalItemName : "");

    if (newItemName.isEmpty()) {
      // Command was canceled.
      result.shouldRedraw = false;
      return result;
    }
  }

  const auto commandResult =
      appBridge->runCommand<neko::FileExplorerCommandResultFfi>(
          CommandType::FileExplorer, commandId, ctx, newItemName.toStdString());

  for (const auto &intent : commandResult.intents) {
    result.intentKinds.push_back(intent.kind);
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
    const auto snapshot = fileTreeBridge->getTreeSnapshot();
    const QString workspaceRootPath = QString::fromUtf8(snapshot.root);

    if (itemPath != workspaceRootPath) {
      fileTreeBridge->setExpanded(parentItemPath.toStdString());
    }
  }

  if (isNewFileCommand || isNewDirectoryCommand ||
      commandId == "fileExplorer.duplicate") {
    fileTreeBridge->setExpanded(parentItemPath.toStdString());
  }

  if (isRenameCommand) {
    if (itemIsExpanded && itemIsDirectory) {
      // Expand the renamed directory.
      QString newItemPath =
          itemFileInfo.dir().path() + QDir::separator() + newItemName;
      fileTreeBridge->setExpanded(newItemPath.toStdString());
    } else if (itemIsExpanded) {
      // Expand the directory containing the renamed file.
      fileTreeBridge->setExpanded(parentItemPath.toStdString());
    }
  }

  return result;
}

void FileExplorerFlows::handleCut(const QString &itemPath) {
  FileIoService::cut(itemPath);
}

void FileExplorerFlows::handleCopy(const QString &itemPath) {
  FileIoService::copy(itemPath);
}

bool FileExplorerFlows::handleDuplicate(const QString &itemPath,
                                        const QString &parentItemPath) {
  const auto result = FileIoService::duplicate(itemPath);

  if (result.success) {
    const auto newItemPath = result.newPath;
    fileTreeBridge->refreshDirectory(parentItemPath.toStdString());
    fileTreeBridge->setCurrent(newItemPath.toStdString());
  }

  return result.success;
}

// TODO(scarlet): Clean this up.
bool FileExplorerFlows::handlePaste(const QString &itemPath,
                                    const QString &parentItemPath) {
  const auto result = FileIoService::paste(itemPath);
  bool destinationIsDirectory = QFileInfo(itemPath).isDir();

  if (result.success) {
    if (result.wasCutOperation) {
      // On a cut/paste, we need to refresh both the destination
      // directory and the source directory.
      if (destinationIsDirectory) {
        fileTreeBridge->refreshDirectory(itemPath.toStdString());

        // TODO(scarlet): Handle multiple selected items eventually.
        QString originalPath = result.items.first().originalPath;
        bool sourceIsDirectory = QFileInfo(originalPath).isDir();

        if (sourceIsDirectory) {
          fileTreeBridge->refreshDirectory(originalPath.toStdString());
          fileTreeBridge->setExpanded(originalPath.toStdString());
        } else {
          const auto originalParentPath = QString::fromUtf8(
              fileTreeBridge->getParentNodePath(originalPath.toStdString()));
          fileTreeBridge->refreshDirectory(originalParentPath.toStdString());
          fileTreeBridge->setExpanded(originalParentPath.toStdString());
        }
      } else {
        fileTreeBridge->refreshDirectory(parentItemPath.toStdString());

        QString originalPath = result.items.first().originalPath;
        bool sourceIsDirectory = QFileInfo(originalPath).isDir();

        if (sourceIsDirectory) {
          fileTreeBridge->refreshDirectory(originalPath.toStdString());
          fileTreeBridge->setExpanded(originalPath.toStdString());
        } else {
          const auto originalParentPath = QString::fromUtf8(
              fileTreeBridge->getParentNodePath(originalPath.toStdString()));
          fileTreeBridge->refreshDirectory(originalParentPath.toStdString());
          fileTreeBridge->setExpanded(originalParentPath.toStdString());
        }
      }
    } else {
      // Otherwise, just refresh the destination directory.
      if (destinationIsDirectory) {
        fileTreeBridge->refreshDirectory(itemPath.toStdString());
      } else {
        fileTreeBridge->refreshDirectory(parentItemPath.toStdString());
      }
    }
  } else {
    // If the paste failed, exit.
    return false;
  }

  // If the destination is a directory, refresh it. Otherwise, refresh the
  // parent directory.
  if (destinationIsDirectory) {
    fileTreeBridge->refreshDirectory(itemPath.toStdString());
  } else {
    fileTreeBridge->refreshDirectory(parentItemPath.toStdString());
  }

  // Avoid collapsing the directory on reload.
  fileTreeBridge->setExpanded(destinationIsDirectory
                                  ? itemPath.toStdString()
                                  : parentItemPath.toStdString());

  QString newItemPath = result.items.first().newPath;
  fileTreeBridge->setCurrent(newItemPath.toStdString());

  return true;
}

void FileExplorerFlows::handleCopyPath(const QString &itemPath) {
  // Path is likely already absolute, but convert it just in case.
  QFileInfo fileInfo(itemPath);

  const auto absolutePath = fileInfo.absoluteFilePath();
  QApplication::clipboard()->setText(absolutePath);
}

void FileExplorerFlows::handleCopyRelativePath(const QString &itemPath) {
  const auto snapshot = fileTreeBridge->getTreeSnapshot();
  const QString workspaceRootPath = QString::fromUtf8(snapshot.root);
  QDir rootDir(workspaceRootPath);

  const QString relativePath = rootDir.relativeFilePath(itemPath);
  QApplication::clipboard()->setText(relativePath);
}

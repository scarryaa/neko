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
  PreCommandProcessingArgs preArgs{
      .currentResult = result,
      .commandId = commandId,
      .itemPath = itemPath,
      .bypassDeleteConfirmation = bypassDeleteConfirmation,
      .itemIsDirectory = itemIsDirectory,
      .itemFileInfo = itemFileInfo,
      .isNewFileCommand = isNewFileCommand,
      .isNewDirectoryCommand = isNewDirectoryCommand,
      .isRenameCommand = isRenameCommand};

  auto preProcessingResult = doPreCommandProcessing(preArgs);
  // Check if the command was canceled.
  if (!preProcessingResult.updatedResult.success) {
    result = preProcessingResult.updatedResult;
    return result;
  }

  const auto commandResult =
      appBridge->runCommand<neko::FileExplorerCommandResultFfi>(
          CommandType::FileExplorer, commandId, ctx,
          preProcessingResult.newItemName.toStdString());

  for (const auto &intent : commandResult.intents) {
    result.intentKinds.push_back(intent.kind);
  }

  PostCommandProcessingArgs postArgs{
      .commandId = commandId,
      .itemPath = itemPath,
      .parentItemPath = parentItemPath,
      .newItemName = preProcessingResult.newItemName,
      .itemFileInfo = itemFileInfo,
      .itemIsExpanded = itemIsExpanded,
      .itemIsDirectory = itemIsDirectory,
      .isNewFileCommand = isNewFileCommand,
      .isNewDirectoryCommand = isNewDirectoryCommand,
      .isRenameCommand = isRenameCommand,
  };

  doPostCommandProcessing(postArgs);

  return result;
}

FileExplorerFlows::PreCommandProcessingResult
FileExplorerFlows::doPreCommandProcessing(
    PreCommandProcessingArgs &args) const {
  PreCommandProcessingResult processingResult{
      .updatedResult = args.currentResult, .newItemName = ""};

  // Open a delete confirmation dialog unless bypassDeleteConfirmation is true.
  if (args.commandId == "fileExplorer.delete" &&
      !args.bypassDeleteConfirmation) {
    using Type = DialogService::DeleteItemType;
    using Decision = DialogService::DeleteDecision;
    Type type = args.itemIsDirectory ? Type::Directory : Type::File;

    const auto deleteResult = DialogService::openDeleteConfirmationDialog(
        args.itemFileInfo.fileName(), type, uiHandles.window);
    if (deleteResult == Decision::Cancel) {
      processingResult.updatedResult.shouldRedraw = false;
      processingResult.updatedResult.success = false;
      return processingResult;
    }
  }

  if (args.isNewFileCommand || args.isNewDirectoryCommand ||
      args.isRenameCommand) {
    using Type = DialogService::OperationType;

    QFileInfo srcInfo(args.itemPath);
    QString originalItemName = srcInfo.fileName();
    Type type;

    if (args.isNewFileCommand) {
      type = Type::NewFile;
    } else if (args.isNewDirectoryCommand) {
      type = Type::NewDirectory;
    } else if (args.isRenameCommand) {
      type = args.itemIsDirectory ? Type::RenameDirectory : Type::RenameFile;
    }

    processingResult.newItemName = DialogService::openItemNameDialog(
        uiHandles.window, type, args.isRenameCommand ? originalItemName : "");

    if (processingResult.newItemName.isEmpty()) {
      // Command was canceled.
      processingResult.updatedResult.shouldRedraw = false;
      processingResult.updatedResult.success = false;
      return processingResult;
    }
  }

  return processingResult;
}

void FileExplorerFlows::doPostCommandProcessing(
    PostCommandProcessingArgs &args) {
  // When creating or modifying an item, unconditionally re-expand the directory
  // containing the item (except for rename operations, we only re-expand if it
  // was expanded to begin with).
  if (args.commandId == "fileExplorer.delete") {
    // Prevent expanding the root's parent (which is not part of the tree -- and
    // the root would be deleted anyway if the deletion target was the root
    // directory).
    // TODO(scarlet): Handle case where root folder is deleted.
    const auto snapshot = fileTreeBridge->getTreeSnapshot();
    const QString workspaceRootPath = QString::fromUtf8(snapshot.root);

    if (args.itemPath != workspaceRootPath) {
      fileTreeBridge->setExpanded(args.parentItemPath.toStdString());
    }
  }

  if (args.isNewFileCommand || args.isNewDirectoryCommand ||
      args.commandId == "fileExplorer.duplicate") {
    fileTreeBridge->setExpanded(args.parentItemPath.toStdString());
  }

  if (args.isRenameCommand) {
    if (args.itemIsExpanded && args.itemIsDirectory) {
      // Expand the renamed directory.
      QString newItemPath =
          args.itemFileInfo.dir().path() + QDir::separator() + args.newItemName;
      fileTreeBridge->setExpanded(newItemPath.toStdString());
    } else if (args.itemIsExpanded) {
      // Expand the directory containing the renamed file.
      fileTreeBridge->setExpanded(args.parentItemPath.toStdString());
    }
  }
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

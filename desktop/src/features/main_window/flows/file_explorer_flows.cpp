#include "file_explorer_flows.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/services/file_io_service.h"
#include "types/command_type.h"
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

  const auto fileExplorerCommand =
      appBridge->parseCommand<CommandType::FileExplorer>(commandId);

  // Handle Qt-side special cases.
  // TODO(scarlet): Move all of these to Rust eventually.
  switch (fileExplorerCommand) {
  case neko::FileExplorerCommandKindFfi::Cut:
    handleCut(itemPath);
    break;
  case neko::FileExplorerCommandKindFfi::Copy:
    handleCopy(itemPath);
    break;
  case neko::FileExplorerCommandKindFfi::Duplicate:
    result.shouldRedraw = true;
    result.success = handleDuplicate(itemPath, parentItemPath);
    break;
  case neko::FileExplorerCommandKindFfi::Paste:
    result.shouldRedraw = true;
    result.success = handlePaste(itemPath, parentItemPath);
    break;
  case neko::FileExplorerCommandKindFfi::CopyPath:
    handleCopyPath(itemPath);
    break;
  case neko::FileExplorerCommandKindFfi::CopyRelativePath:
    handleCopyRelativePath(itemPath);
    break;
  default:
    // The rest of the commands are handled via Rust; we do some processing
    // first before handing them off. To avoid returning early if there wasn't a
    // previous match, set 'success' to true.
    result.success = true;
    break;
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

  QFileInfo itemFileInfo(itemPath);
  bool itemIsDirectory = itemFileInfo.isDir();
  PreCommandProcessingArgs preArgs{.currentResult = result,
                                   .commandId = commandId,
                                   .itemPath = itemPath,
                                   .bypassDeleteConfirmation =
                                       bypassDeleteConfirmation,
                                   .itemIsDirectory = itemIsDirectory,
                                   .itemFileInfo = itemFileInfo,
                                   .fileExplorerCommand = fileExplorerCommand};

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
      .fileExplorerCommand = fileExplorerCommand};

  doPostCommandProcessing(postArgs);

  return result;
}

FileExplorerFlows::PreCommandProcessingResult
FileExplorerFlows::doPreCommandProcessing(
    PreCommandProcessingArgs &args) const {
  using CommandKind = neko::FileExplorerCommandKindFfi;

  PreCommandProcessingResult processingResult{
      .updatedResult = args.currentResult, .newItemName = ""};

  // Open a delete confirmation dialog unless bypassDeleteConfirmation is true.
  if (args.fileExplorerCommand == CommandKind::Delete &&
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

  bool isNewFileCommand = args.fileExplorerCommand == CommandKind::NewFile;
  bool isNewDirectoryCommand =
      args.fileExplorerCommand == CommandKind::NewFolder;
  bool isRenameCommand = args.fileExplorerCommand == CommandKind::Rename;
  if (isNewFileCommand || isNewDirectoryCommand || isRenameCommand) {
    using Type = DialogService::OperationType;

    QFileInfo srcInfo(args.itemPath);
    QString originalItemName = srcInfo.fileName();
    Type type;

    if (isNewFileCommand) {
      type = Type::NewFile;
    } else if (isNewDirectoryCommand) {
      type = Type::NewDirectory;
    } else if (isRenameCommand) {
      type = args.itemIsDirectory ? Type::RenameDirectory : Type::RenameFile;
    }

    processingResult.newItemName = DialogService::openItemNameDialog(
        uiHandles.window, type, isRenameCommand ? originalItemName : "");

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
  using CommandKind = neko::FileExplorerCommandKindFfi;

  // When creating or modifying an item, unconditionally re-expand the directory
  // containing the item (except for rename operations, we only re-expand if it
  // was expanded to begin with).
  if (args.fileExplorerCommand == CommandKind::Delete) {
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

  // If a new file/directory was created/duplicated, expand the parent
  // directory.
  if (args.fileExplorerCommand == CommandKind::NewFile ||
      args.fileExplorerCommand == CommandKind::NewFolder ||
      args.fileExplorerCommand == CommandKind::Duplicate) {
    fileTreeBridge->setExpanded(args.parentItemPath.toStdString());
  }

  // If it was a rename command and the target item was originally expanded,
  // expand it again.
  if (args.fileExplorerCommand == CommandKind::Rename) {
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

// NOLINTNEXTLINE
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

// TODO(scarlet): Handle case where a directory or file is cut/copied and pasted
// in the same directory.
bool FileExplorerFlows::handlePaste(const QString &itemPath,
                                    const QString &parentItemPath) {
  // Attempt the paste operation.
  const auto result = FileIoService::paste(itemPath);

  if (!result.success) {
    return false;
  }

  // If it was a cut/paste operation, refresh the source directory.
  if (result.wasCutOperation && !result.items.isEmpty()) {
    // TODO(scarlet): Handle multiple selected items eventually.
    refreshSourceAfterCut(result.items.first().originalPath);
  }

  // Refresh and expand the destination directory.
  std::string destinationPathToRefresh =
      resolveRefreshPath(itemPath, parentItemPath);
  fileTreeBridge->refreshDirectory(destinationPathToRefresh);
  fileTreeBridge->setExpanded(destinationPathToRefresh);

  // Select the pasted item.
  if (!result.items.isEmpty()) {
    QString newItemPath = result.items.first().newPath;
    fileTreeBridge->setCurrent(newItemPath.toStdString());
  }

  return true;
}

// Determines whether to use the itemPath (if it's a directory)
// or the parentPath.
std::string FileExplorerFlows::resolveRefreshPath(const QString &itemPath,
                                                  const QString &parentPath) {
  QFileInfo fileInfo(itemPath);

  if (fileInfo.isDir()) {
    return itemPath.toStdString();
  }

  return parentPath.toStdString();
}

// Handles refreshing the source location after a 'cut' operation.
void FileExplorerFlows::refreshSourceAfterCut(const QString &originalPath) {
  QFileInfo originalPathInfo(originalPath);

  // If the original path is a directory, just refresh it.
  if (originalPathInfo.isDir()) {
    std::string path = originalPath.toStdString();

    fileTreeBridge->refreshDirectory(path);
    fileTreeBridge->setExpanded(path);
  } else {
    // If the original path is not a directory, calculate the parent directory
    // of the original file and refresh that.
    std::string parentPath =
        fileTreeBridge->getParentNodePath(originalPath.toStdString());

    fileTreeBridge->refreshDirectory(parentPath);
    fileTreeBridge->setExpanded(parentPath);
  }
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

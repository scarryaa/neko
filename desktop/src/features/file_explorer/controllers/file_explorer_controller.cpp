#include "file_explorer_controller.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/services/file_io_service.h"

// TODO(scarlet): Cache tree snapshots?
FileExplorerController::FileExplorerController(
    const FileExplorerControllerProps &props, QObject *parent)
    : fileTreeBridge(props.fileTreeBridge), QObject(parent) {}

// Loads the provided directory path and expands it. I.e. initializes the file
// tree.
void FileExplorerController::loadDirectory(const QString &rootDirectoryPath) {
  fileTreeBridge->setRootDirectory(rootDirectoryPath);
  setExpanded(rootDirectoryPath);

  emit rootDirectoryChanged(rootDirectoryPath);
}

// Toggles the provided directory path expanded/collapsed state.
void FileExplorerController::toggleExpanded(const QString &directoryPath) {
  fileTreeBridge->toggleExpanded(directoryPath);
}

// Sets the provided directory path to expanded.
void FileExplorerController::setExpanded(const QString &directoryPath) {
  fileTreeBridge->setExpanded(directoryPath);
}

// Sets the provided directory path to collapsed.
void FileExplorerController::setCollapsed(const QString &directoryPath) {
  fileTreeBridge->setCollapsed(directoryPath);
}

// Attempts to retrieve the first node in the tree.
FileExplorerController::FileNodeInfo FileExplorerController::getFirstNode() {
  auto maybeFirstNode = fileTreeBridge->getFirstNode();

  // First node was found and is valid.
  if (maybeFirstNode.found_node) {
    return {.index = 0, .nodeSnapshot = maybeFirstNode.node};
  }

  // First node was not found.
  return {.index = -1};
}

// Attempts to retrieve the last node in the tree.
FileExplorerController::FileNodeInfo FileExplorerController::getLastNode() {
  auto maybeLastNode = fileTreeBridge->getLastNode();

  // Last node was found and is valid.
  if (maybeLastNode.found_node) {
    return {.index = getNodeCount() - 1, .nodeSnapshot = maybeLastNode.node};
  }

  // Last node was not found.
  return {.index = -1};
}

// Returns the number of nodes in the tree.
int FileExplorerController::getNodeCount() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();

  return static_cast<int>(snapshot.nodes.size());
}

// Returns a snapshot of the current tree state.
neko::FileTreeSnapshot FileExplorerController::getTreeSnapshot() {
  return fileTreeBridge->getTreeSnapshot();
}

// Clears the current selected/focused node.
void FileExplorerController::clearSelection() {
  fileTreeBridge->clearCurrent();
}

neko::FileExplorerContextFfi FileExplorerController::getCurrentContext() {
  auto currentNode = getNode([](const auto &node) { return node.is_current; });

  // If the found node is valid, construct and return the context.
  if (currentNode.foundNode()) {
    return neko::FileExplorerContextFfi{
        .item_path = currentNode.nodeSnapshot.path,
        .item_is_directory = currentNode.nodeSnapshot.is_dir,
        .item_is_expanded = currentNode.nodeSnapshot.is_expanded};
  }

  // Otherwise, return an empty context.
  return neko::FileExplorerContextFfi{};
}

// Handles a 'cut' operation.
//
// Retrieves the current node information, and then calls `FileIoService` to
// perform the actual operation.
void FileExplorerController::handleCut() {
  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });

  // If the node was found, perform the cut.
  if (nodeInfo.foundNode()) {
    FileIoService::cut(nodeInfo.nodeSnapshot.path.c_str());
  }
}

// Handles a 'copy' operation.
//
// Retrieves the current node information, and then calls `FileIoService` to
// perform the actual operation.
void FileExplorerController::handleCopy() {
  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });

  // If the node was found, perform the copy.
  if (nodeInfo.foundNode()) {
    FileIoService::copy(nodeInfo.nodeSnapshot.path.c_str());
  }
}

// Handles a 'paste' operation.
//
// Retrieves the current node information, and then calls `FileIoService` to
// perform the actual operation.
//
// After, it triggers a refresh of the target directory to make sure the new
// items appear.
void FileExplorerController::handlePaste() {
  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });

  // If the node was not found, return.
  if (!nodeInfo.foundNode()) {
    return;
  }

  // Determine whether to target the parent node or the current node, based on
  // whether the current node is a directory.
  auto parentNodePath =
      fileTreeBridge->getParentNodePath(nodeInfo.nodeSnapshot.path.c_str());
  QString targetDirectory = nodeInfo.nodeSnapshot.is_dir
                                ? nodeInfo.nodeSnapshot.path.c_str()
                                : parentNodePath;

  auto pasteResult = FileIoService::paste(targetDirectory);

  // If the paste was successful, refresh the parent directory and select the
  // new item.
  if (pasteResult.success) {
    // If the original item was deleted, remove it from the clipboard.
    if (pasteResult.originalWasDeleted) {
    }
    fileTreeBridge->refreshDirectory(parentNodePath);
    fileTreeBridge->setCurrent(pasteResult.items.first().newPath);
  }
}

// Handles a 'duplicate' operation.
//
// Retrieves the current node information, and then calls `FileIoService` to
// perform the actual operation.
//
// After, it triggers a refresh of the target directory to make sure the new
// items appear.
void FileExplorerController::handleDuplicate() {
  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });

  // If the node was not found, return.
  if (!nodeInfo.foundNode()) {
    return;
  }

  auto currentNodePath = QString(nodeInfo.nodeSnapshot.path.c_str());
  auto parentNodePath = fileTreeBridge->getParentNodePath(currentNodePath);

  const auto duplicateResult = FileIoService::duplicate(currentNodePath);

  // If the duplicate was successful, refresh the parent directory, and select
  // the duplicated node.
  if (duplicateResult.success) {
    fileTreeBridge->refreshDirectory(parentNodePath);
    fileTreeBridge->setCurrent(duplicateResult.newPath);
  }
}

// Initiates a 'delete' operation.
//
// First, it retrieves and validates the current node information.
//
// Then, it shows a confirmation dialog if requested, then proceeds with the
// actual deletion (see `deleteItem`).
void FileExplorerController::handleDelete(bool shouldConfirm) {
  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });

  // If the node was not found, return.
  if (!nodeInfo.foundNode()) {
    return;
  }

  using Decision = DialogService::DeleteDecision;

  auto currentNode = nodeInfo.nodeSnapshot;
  DialogService::DeleteDecision decision;

  if (shouldConfirm) {
    using ItemType = DialogService::DeleteItemType;

    ItemType itemType =
        currentNode.is_dir ? ItemType::Directory : ItemType::File;

    decision = DialogService::openDeleteConfirmationDialog(
        currentNode.name.c_str(), itemType,
        reinterpret_cast<QWidget *>(parent()));
  }

  if (decision == Decision::Delete || !shouldConfirm) {
    deleteItem(currentNode.path.c_str(), currentNode);
  }
}

// Performs a 'delete' operation.
//
// Calls `FileIoService` to perform the actual delete operation.
//
// If the deletion was successful, it retrieves the relevant node/parent node
// information, refreshes and re-expands the parent node/directory, and
// focuses the previous node in the tree.
void FileExplorerController::deleteItem(
    const QString &path, const neko::FileNodeSnapshot &currentNode) {
  // Retrieve the first node to later check if the deleted item was the very
  // first node.
  auto originalFirstNode = getFirstNode();
  bool wasSuccessful = FileIoService::deleteItem(path);

  // If the deletion succeeded, update the tree state.
  if (wasSuccessful) {
    auto parentPath = fileTreeBridge->getParentNodePath(path);

    fileTreeBridge->refreshDirectory(parentPath);
    fileTreeBridge->setExpanded(parentPath);

    // If the deleted node was the first tree item, get the new first item
    // instead.
    if (originalFirstNode.foundNode() &&
        originalFirstNode.nodeSnapshot.path == path) {
      auto newFirstNode = getFirstNode();

      if (newFirstNode.foundNode()) {
        fileTreeBridge->setCurrent(newFirstNode.nodeSnapshot.path.c_str());
      }
    } else {
      // Check if the deleted item was the first child of a directory.
      if (path.contains(parentPath)) {
        // If so, set the current node to the parent node.
        fileTreeBridge->setCurrent(parentPath);
      } else {
        // Otherwise, get the previous node and set it as current.
        auto previousNode = fileTreeBridge->getPreviousNode(path);

        fileTreeBridge->setCurrent(previousNode.path.c_str());
      }
    }
  }
}

FileExplorerController::SelectFirstTreeNodeResult
FileExplorerController::selectFirstTreeNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  int nodeCount = static_cast<int>(snapshot.nodes.size());

  // If the tree is not empty, select the first node in the tree.
  if (nodeCount > 0) {
    fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());

    return {.nodeChanged = true, .nodePath = snapshot.nodes[0].path.c_str()};
  }

  // Otherwise, do nothing.
  return {.nodeChanged = false};
}

FileExplorerController::CheckValidNodeResult
FileExplorerController::checkForValidNode(
    FileExplorerController::FileNodeInfo &nodeInfo) {
  // Make sure the current node has a valid path/exists.
  if (nodeInfo.foundNode()) {
    return {.action = Action::None,
            .validNode = true,
            .nodePath = nodeInfo.nodeSnapshot.path.c_str()};
  }

  // Current node was not found or does not exist, so select the first node in
  // the tree and exit.
  auto selectResult = selectFirstTreeNode();

  return {.action = Action::None,
          .validNode = false,
          .nodePath = selectResult.nodePath};
}

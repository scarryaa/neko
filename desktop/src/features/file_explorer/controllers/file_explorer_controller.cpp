#include "file_explorer_controller.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/services/file_io_service.h"

// TODO(scarlet): Cache tree snapshots?
FileExplorerController::FileExplorerController(
    const FileExplorerControllerProps &props, QObject *parent)
    : fileTreeBridge(props.fileTreeBridge), QObject(parent) {}

void FileExplorerController::loadDirectory(const QString &rootDirectoryPath) {
  fileTreeBridge->setRootDirectory(rootDirectoryPath);
  setExpanded(rootDirectoryPath);

  emit rootDirectoryChanged(rootDirectoryPath);
}

void FileExplorerController::setExpanded(const QString &directoryPath) {
  fileTreeBridge->setExpanded(directoryPath);
}

void FileExplorerController::setCollapsed(const QString &directoryPath) {
  fileTreeBridge->setCollapsed(directoryPath);
}

void FileExplorerController::toggleExpanded(const QString &directoryPath) {
  fileTreeBridge->toggleExpanded(directoryPath);
}

FileExplorerController::FileNodeInfo FileExplorerController::getFirstNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  int index = 0;

  auto firstNode = snapshot.nodes.front();

  return {.index = index, .nodeSnapshot = firstNode};
}

FileExplorerController::FileNodeInfo FileExplorerController::getLastNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();

  int index = static_cast<int>(snapshot.nodes.size() - 1);
  auto lastNode = snapshot.nodes.back();

  return {.index = index, .nodeSnapshot = lastNode};
}

int FileExplorerController::getNodeCount() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();

  return static_cast<int>(snapshot.nodes.size());
}

neko::FileTreeSnapshot FileExplorerController::getTreeSnapshot() {
  return fileTreeBridge->getTreeSnapshot();
}

void FileExplorerController::clearSelection() {
  fileTreeBridge->clearCurrent();
}

FileExplorerController::ClickResult
FileExplorerController::handleNodeClick(int index, bool isLeftClick) {
  auto snapshot = fileTreeBridge->getTreeSnapshot();

  // Check if the index is out of bounds.
  if (index < 0 || index >= snapshot.nodes.size()) {
    return {Action::None};
  }

  const auto &node = snapshot.nodes[index];
  const QString &nodePath = QString::fromUtf8(node.path);

  // Set the current selected node.
  fileTreeBridge->setCurrent(nodePath);

  // Return if it is a right click.
  if (!isLeftClick) {
    return {Action::None};
  }

  // If the clicked on node is a directory, toggle it.
  if (node.is_dir) {
    fileTreeBridge->toggleExpanded(nodePath);
    return {Action::LayoutChanged};
  }

  // Otherwise, select the clicked on file node.
  return {Action::FileSelected, nodePath};
}

// Handles the 'Enter' keypress event.
//
// If a valid node is selected:
// - If it's a directory, the expansion state is toggled.
// - If it's a file, it's marked to be opened.
//
// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleEnter() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  // If the current node is a directory, toggle it.
  if (currentNode.is_dir) {
    toggleExpanded(currentNodePath);

    return {.action = Action::LayoutChanged, .filePath = currentNodePath};
  }

  // Otherwise, signal to open the current node (file) in the editor.
  return {.action = Action::FileSelected, .filePath = currentNodePath};
}

// Handles the 'Space' keypress event.
//
// If a valid node is focused, it is selected/deselected.
//
// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleSpace() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  // If the current node is valid, toggle the selection state.
  fileTreeBridge->toggleSelect(currentNodePath);

  return {.action = Action::None, .filePath = currentNodePath};
}

// Handles the 'Left' arrow keypress event.
//
// If a valid node is selected:
// - If it's a directory:
//    - Expanded: Set to collapsed.
//    - Collapsed: Move to the parent node (directory), select it, and collapse
//    it.
//
// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleLeft() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  // If current node is collapsed, go to the parent node (directory), select it,
  // and collapse it.
  if (!currentNode.is_expanded) {
    auto rootNodePath = fileTreeBridge->getRootPath();
    auto parentNodePath = fileTreeBridge->getParentNodePath(currentNodePath);

    if (rootNodePath == parentNodePath) {
      return {.action = Action::None, .filePath = currentNodePath};
    }

    fileTreeBridge->setCurrent(parentNodePath);
    fileTreeBridge->setCollapsed(parentNodePath);

    return {.action = Action::LayoutChanged, .filePath = parentNodePath};
  }

  // Otherwise, if the current node is expanded, just collapse it.
  setCollapsed(currentNodePath);

  return {.action = Action::LayoutChanged, .filePath = currentNodePath};
}

// Handles the 'Right' arrow keypress event.
//
// If a valid node is selected:
// - If it's a directory:
//    - Expanded: Move to the first child node and select it.
//    - Collapsed: Set to expanded.
//
// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleRight() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  // If current node is collapsed, expand it.
  if (!currentNode.is_expanded) {
    setExpanded(currentNodePath);

    return {.action = Action::LayoutChanged, .filePath = currentNodePath};
  }

  // Otherwise, if the current node is expanded and it has children, move to the
  // first child and select it.
  auto children = fileTreeBridge->getVisibleChildren(currentNodePath);

  if (!children.empty()) {
    fileTreeBridge->setCurrent(children[0].path.c_str());

    return {.action = Action::None, .filePath = children[0].path.c_str()};
  }

  // TODO(scarlet): What do we do when the current node does not have children?
  return {.action = Action::None, .filePath = currentNodePath};
}

// Handles the 'Up' arrow keypress event.
//
// Attempts to move the selection to the previous node in the tree. If at the
// very first node, it wraps around to the end of the tree.
//
// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleUp() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  auto firstNodeInfo = getFirstNode();

  // If we failed to get the first node, exit.
  if (!firstNodeInfo.foundNode()) {
    return {.action = Action::None, .filePath = currentNodePath};
  }

  // If at the top of the tree, wrap to the end of the tree.
  if (currentNodePath == firstNodeInfo.nodeSnapshot.path) {
    auto lastNodeInfo = getLastNode();

    // If we failed to get the last node, exit.
    if (!lastNodeInfo.foundNode()) {
      return {.action = Action::None, .filePath = currentNodePath};
    }

    fileTreeBridge->setCurrent(lastNodeInfo.nodeSnapshot.path.c_str());

    return {.action = Action::None,
            .filePath = lastNodeInfo.nodeSnapshot.path.c_str()};
  }

  // Otherwise, select the previous node.
  // TODO(scarlet): Update the Rust fn to wrap around?
  auto previousNode = fileTreeBridge->getPreviousNode(currentNodePath);
  fileTreeBridge->setCurrent(previousNode.path.c_str());

  return {.action = Action::None, .filePath = previousNode.path.c_str()};
}

// Handles the 'Down' arrow keypress event.
//
// Attempts to move the selection to the next node in the tree. If at the
// very last node, it wraps around to the beginning of the tree.

// If the node does not exist or is otherwise invalid, the first tree node is
// selected (if possible).
FileExplorerController::KeyboardResult FileExplorerController::handleDown() {
  neko::FileNodeSnapshot currentNode;
  QString currentNodePath;

  auto nodeInfo = getNode([](const auto &node) { return node.is_current; });
  auto validNodeResult = checkForValidNode(nodeInfo);

  if (validNodeResult.validNode) {
    // Current node was valid, continue.
    currentNodePath = validNodeResult.nodePath;
    currentNode = nodeInfo.nodeSnapshot;
  } else {
    // Current node was invalid and was updated, so return.
    return {.action = Action::None, .filePath = validNodeResult.nodePath};
  }

  auto lastNodeInfo = getLastNode();

  // If we failed to get the last node, exit.
  if (!lastNodeInfo.foundNode()) {
    return {.action = Action::None, .filePath = currentNodePath};
  }

  // If at the bottom of the tree, wrap to the beginning of the tree.
  if (currentNodePath == lastNodeInfo.nodeSnapshot.path) {
    auto firstNodeInfo = getFirstNode();

    // If we failed to get the first node, exit.
    if (!firstNodeInfo.foundNode()) {
      return {.action = Action::None, .filePath = currentNodePath};
    }

    fileTreeBridge->setCurrent(firstNodeInfo.nodeSnapshot.path.c_str());

    return {.action = Action::None,
            .filePath = firstNodeInfo.nodeSnapshot.path.c_str()};
  }

  // Otherwise, select the next node.
  // TODO(scarlet): Update the Rust fn to wrap around?
  auto nextNode = fileTreeBridge->getNextNode(currentNodePath);
  fileTreeBridge->setCurrent(nextNode.path.c_str());

  return {.action = Action::None, .filePath = nextNode.path.c_str()};
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

  // TODO(scarlet): return action results?
  // return { ... };
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

  // TODO(scarlet): return action results?
  // return { ... };
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

  FileIoService::paste(targetDirectory);

  fileTreeBridge->refreshDirectory(targetDirectory);

  // TODO(scarlet): return action results?
  // return { ... };
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

  // TODO(scarlet): return action results?
  // return { ... };
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
  bool wasSuccessful = FileIoService::deleteItem(path);

  // If the deletion succeeded, update the tree state.
  if (wasSuccessful) {
    auto previousNode = fileTreeBridge->getPreviousNode(path);
    auto parentPath = fileTreeBridge->getParentNodePath(path);
    bool currentIsDir = currentNode.is_dir;

    fileTreeBridge->refreshDirectory(parentPath);
    fileTreeBridge->setExpanded(parentPath);
    fileTreeBridge->setCurrent(previousNode.path.c_str());
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

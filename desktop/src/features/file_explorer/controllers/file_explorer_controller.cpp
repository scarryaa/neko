#include "file_explorer_controller.h"
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
  auto pasteInfo = FileIoService::getClipboardItems();
  rust::Vec<neko::PasteItemFfi> rustPasteItems;

  for (const auto &item : pasteInfo.items) {
    rustPasteItems.push_back(
        {.path = item.path.toStdString(), .is_dir = item.isDirectory});
  }

  // If the found node is valid, construct and return the context.
  if (currentNode.foundNode()) {
    return neko::FileExplorerContextFfi{
        .item_path = currentNode.nodeSnapshot.path,
        .item_is_directory = currentNode.nodeSnapshot.is_dir,
        .item_is_expanded = currentNode.nodeSnapshot.is_expanded,
        .paste_info = {.items = rustPasteItems,
                       .is_cut_operation = pasteInfo.isCutOperation}};
  }

  // Otherwise, return an empty context.
  //
  // We still include the paste info/items, since pasting without a current node
  // is considered to be a paste into the root directory.
  return neko::FileExplorerContextFfi{
      .paste_info = {.items = rustPasteItems,
                     .is_cut_operation = pasteInfo.isCutOperation}};
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

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

// Sets the current node to the provided path.
void FileExplorerController::setCurrent(const QString &targetPath) {
  fileTreeBridge->setCurrent(targetPath);
}

// Sets the provided directory path to expanded.
void FileExplorerController::setExpanded(const QString &directoryPath) {
  fileTreeBridge->setExpanded(directoryPath);
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

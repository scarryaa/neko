#include "file_explorer_controller.h"

#include "features/file_explorer/types/types.h"
#include "features/main_window/services/file_io_service.h"
#include <utility>

// TODO(scarlet): Cache tree snapshots?
// TODO(scarlet): Add customizable keybindings/vim keybinds.
FileExplorerController::FileExplorerController(
    const FileExplorerControllerProps &props, QObject *parent)
    : fileTreeBridge(props.fileTreeBridge), QObject(parent) {
  // Copy/Cut/Paste/Duplicate operations.
  bind(
      QKeyCombination(Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier,
                      Qt::Key_C),
      [this] { triggerCommand("fileExplorer.copyRelativePath"); });
  bind(QKeyCombination(Qt::ControlModifier | Qt::AltModifier, Qt::Key_C),
       [this] { triggerCommand("fileExplorer.copyPath"); });
  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_C),
       [this] { handleCopy(); });

  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_V),
       [this] { triggerCommand("fileExplorer.paste"); });

  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_X),
       [this] { triggerCommand("fileExplorer.cut"); });

  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_D),
       [this] { triggerCommand("fileExplorer.duplicate"); });

  // Font operations.
  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_Equal),
       [this] { fontSizeAdjustment = FontSizeAdjustment::Increase; });
  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_Minus),
       [this] { fontSizeAdjustment = FontSizeAdjustment::Decrease; });
  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_0),
       [this] { fontSizeAdjustment = FontSizeAdjustment::Reset; });

  // Find in folder.
  bind(QKeyCombination(Qt::ControlModifier, Qt::Key_Backslash),
       [this] { triggerCommand("fileExplorer.findInFolder"); });

  // Navigation operations.
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Up), [this] {
    triggerCommand("fileExplorer.navigateUp");
    needsScroll = true;
  });
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Down), [this] {
    triggerCommand("fileExplorer.navigateDown");
    needsScroll = true;
  });
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Left), [this] {
    triggerCommand("fileExplorer.navigateLeft");
    needsScroll = true;
  });
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Right), [this] {
    triggerCommand("fileExplorer.navigateRight");
    needsScroll = true;
  });

  // Node modification/selection operations.
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Space), [this] {
    // Toggle select for this node.
    // TODO(scarlet): Add support for operations on multiple nodes.
    triggerCommand("fileExplorer.toggleSelect");
  });

  auto action = [this] {
    // Toggle expand/collapse for this node if it's a directory, or open it if
    // it's a file.
    emit requestFocusEditor(true);
    triggerCommand("fileExplorer.action");
  };
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Return), action);
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Enter), action);
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_E), action);

  bind(QKeyCombination(Qt::ShiftModifier, Qt::Key_Delete), [this] {
    // Delete and skip the delete confirmation dialog.
    triggerCommand("fileExplorer.delete", true);
  });
  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Delete), [this] {
    // Delete, but show the delete confirmation dialog.
    triggerCommand("fileExplorer.delete");
  });

  bind(QKeyCombination(Qt::ShiftModifier, Qt::Key_R),
       [this] { triggerCommand("fileExplorer.rename"); });

  bind(QKeyCombination(Qt::NoModifier, Qt::Key_X),
       [this] { triggerCommand("fileExplorer.reveal"); });

  bind(QKeyCombination(Qt::ShiftModifier, Qt::Key_D), [this] {
    // Delete, but show the delete confirmation dialog.
    triggerCommand("fileExplorer.delete");
  });

  bind(QKeyCombination(Qt::NoModifier, Qt::Key_D),
       [this] { triggerCommand("fileExplorer.newFolder"); });

  bind(QKeyCombination(Qt::ShiftModifier, Qt::Key_Percent),
       [this] { triggerCommand("fileExplorer.newFile"); });

  bind(QKeyCombination(Qt::ShiftModifier, Qt::Key_C),
       [this] { triggerCommand("fileExplorer.collapseAll"); });

  bind(QKeyCombination(Qt::NoModifier, Qt::Key_Escape),
       [this] { triggerCommand("fileExplorer.clearSelected"); });
}

void FileExplorerController::bind(QKeyCombination combo,
                                  std::function<void()> action) {
  keyMappings[combo] = std::move(action);
}

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
        .target_is_item = true,
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
      .item_path = fileTreeBridge->getRootPath().toStdString(),
      .item_is_directory = true,
      .item_is_expanded = true,
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

void FileExplorerController::triggerCommand(
    const std::string &commandId, bool bypassDeleteConfirmation, int index,
    const TargetNodePath &targetNodePath,
    const DestinationNodePath &destinationNodePath) {
  // Retreive the context.
  auto ctx = getCurrentContext();
  ctx.index = index;
  ctx.move_target_node_path = targetNodePath.value;
  ctx.move_destination_node_path = destinationNodePath.value;

  emit commandRequested(commandId, ctx, bypassDeleteConfirmation);
}

ChangeSet
FileExplorerController::handleKeyPress(int key,
                                       Qt::KeyboardModifiers modifiers) {
  auto normalizedMods = modifiers & ~Qt::KeypadModifier;
  QKeyCombination lookupId = QKeyCombination(normalizedMods, Qt::Key(key));
  ChangeSet changeSet = {.scroll = false,
                         .redraw = false,
                         .fontSizeAdjustment = FontSizeAdjustment::NoChange};

  if (keyMappings.count(lookupId) != 0U) {
    keyMappings[lookupId]();

    changeSet.redraw = true;

    if (needsScroll) {
      changeSet.scroll = true;
      needsScroll = false;
    }

    changeSet.fontSizeAdjustment = fontSizeAdjustment;
    fontSizeAdjustment = FontSizeAdjustment::NoChange;
  }

  return changeSet;
}

FileNodeInfo FileExplorerController::handleNodeClick(int row,
                                                     bool wasLeftMouseButton) {
  auto targetNode = getNodeByIndex(row);

  // If clicking on an empty space, clear the current node.
  if (!targetNode.foundNode()) {
    setCurrent("");
    return {};
  }

  // If it was a non-left button click, just update the current node.
  //
  // Normally this is done on mouse release, but we do it here to ensure the
  // context menu has the correct node.
  if (!wasLeftMouseButton) {
    setCurrent(QString::fromUtf8(targetNode.nodeSnapshot.path));
    return {};
  }

  // Mark the dragged node ahead of time, to prevent selecting the wrong node
  // later.
  if (targetNode.foundNode()) {
    return targetNode;
  }

  return {};
}

void FileExplorerController::handleNodeClickRelease(int row,
                                                    bool wasLeftMouseButton) {
  const auto targetNode = getNodeByIndex(row);

  // If the click was not the left mouse button, only select the target node.
  if (!wasLeftMouseButton) {
    setCurrent(QString::fromUtf8(targetNode.nodeSnapshot.path));

    return;
  }

  // Trigger an action but do NOT focus the editor (if opening a file).
  //
  // If a double click is pending, do not do anything, just reset the flag.
  if (doubleClickPending) {
    doubleClickPending = false;
    return;
  }

  triggerCommand("fileExplorer.actionIndex", false, row);
}

void FileExplorerController::handleNodeDoubleClick(int row,
                                                   bool wasLeftMouseButton) {
  // If the click was not the left mouse button, don't do anything.
  if (!wasLeftMouseButton) {
    return;
  }

  // Trigger an action AND focus the editor (if opening a file).
  emit requestFocusEditor(true);
  triggerCommand("fileExplorer.actionIndex", false, row);

  doubleClickPending = true;
}

void FileExplorerController::handleNodeDrop(int row,
                                            const QByteArray &encodedData) {
  const auto targetNode = getNodeByIndex(row);
  const std::string sourcePath = encodedData.toStdString();

  // Pass an empty string to allow for moving an item to the root directory.
  const std::string destinationPath =
      targetNode.foundNode() ? std::string(targetNode.nodeSnapshot.path) : "";

  triggerCommand("fileExplorer.move", false, -1, TargetNodePath{sourcePath},
                 DestinationNodePath{destinationPath});
}

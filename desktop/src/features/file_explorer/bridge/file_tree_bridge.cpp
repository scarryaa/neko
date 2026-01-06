#include "file_tree_bridge.h"

// TODO(scarlet): Ideally convert all ::rust::String returns to c_str or
// QString.
FileTreeBridge::FileTreeBridge(FileTreeBridgeProps props, QObject *parent)
    : QObject(parent), fileTreeController(std::move(props.fileTreeController)) {
}

neko::FileTreeSnapshot FileTreeBridge::getTreeSnapshot() {
  return fileTreeController->get_tree_snapshot();
}

QString FileTreeBridge::getParentNodePath(const QString &path) {
  return fileTreeController->get_path_of_parent(path.toStdString()).c_str();
}

std::vector<neko::FileNodeSnapshot>
FileTreeBridge::getVisibleChildren(const QString &directoryPath) {
  std::vector<neko::FileNodeSnapshot> children = {};
  rust::Vec<neko::FileNodeSnapshot> rawChildren =
      fileTreeController->get_children(directoryPath.toStdString());

  if (!rawChildren.empty()) {
    for (const auto &child : rawChildren) {
      children.push_back(child);
    }
  }

  return children;
}

neko::FileNodeSnapshot
FileTreeBridge::getPreviousNode(const QString &currentNodePath) {
  return fileTreeController->get_prev_node(currentNodePath.toStdString());
}

neko::FileNodeSnapshot
FileTreeBridge::getNextNode(const QString &currentNodePath) {
  return fileTreeController->get_next_node(currentNodePath.toStdString());
}

void FileTreeBridge::setRootDirectory(const QString &rootDirectoryPath) {
  fileTreeController->set_root_path(rootDirectoryPath.toStdString());
}

void FileTreeBridge::setExpanded(const QString &directoryPath) {
  fileTreeController->set_expanded(directoryPath.toStdString());
}

void FileTreeBridge::setCurrent(const QString &itemPath) {
  fileTreeController->set_current_path(itemPath.toStdString());
}

void FileTreeBridge::clearCurrent() {
  fileTreeController->clear_current_path();
}

void FileTreeBridge::toggleExpanded(const QString &directoryPath) {
  fileTreeController->toggle_expanded(directoryPath.toStdString());
}

void FileTreeBridge::toggleSelect(const QString &nodePath) {
  fileTreeController->toggle_select_for_path(nodePath.toStdString());
}

void FileTreeBridge::setCollapsed(const QString &directoryPath) {
  fileTreeController->set_collapsed(directoryPath.toStdString());
}

void FileTreeBridge::refreshDirectory(const QString &directoryPath) {
  fileTreeController->refresh_dir(directoryPath.toStdString());
}

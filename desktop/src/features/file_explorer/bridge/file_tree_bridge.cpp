#include "file_tree_bridge.h"

FileTreeBridge::FileTreeBridge(FileTreeBridgeProps props, QObject *parent)
    : QObject(parent), fileTreeController(std::move(props.fileTreeController)) {
}

neko::FileTreeSnapshot FileTreeBridge::getTreeSnapshot() {
  return fileTreeController->get_tree_snapshot();
}

std::string FileTreeBridge::getParentNodePath(const std::string &path) {
  return fileTreeController->get_path_of_parent(path).c_str();
}

std::vector<neko::FileNodeSnapshot>
FileTreeBridge::getVisibleChildren(const std::string &directoryPath) {
  std::vector<neko::FileNodeSnapshot> children = {};
  rust::Vec<neko::FileNodeSnapshot> rawChildren =
      fileTreeController->get_children(directoryPath);

  if (!rawChildren.empty()) {
    for (const auto &child : rawChildren) {
      children.push_back(child);
    }
  }

  return children;
}

neko::FileNodeSnapshot
FileTreeBridge::getPreviousNode(const std::string &currentNodePath) {
  return fileTreeController->get_prev_node(currentNodePath);
}

neko::FileNodeSnapshot
FileTreeBridge::getNextNode(const std::string &currentNodePath) {
  return fileTreeController->get_next_node(currentNodePath);
}

void FileTreeBridge::setRootDir(const std::string &path) {
  fileTreeController->set_root_path(path);
}

void FileTreeBridge::setExpanded(const std::string &directoryPath) {
  fileTreeController->set_expanded(directoryPath);
}

void FileTreeBridge::setCurrent(const std::string &itemPath) {
  fileTreeController->set_current_path(itemPath);
}

void FileTreeBridge::clearCurrent() {
  fileTreeController->clear_current_path();
}

void FileTreeBridge::toggleExpanded(const std::string &directoryPath) {
  fileTreeController->toggle_expanded(directoryPath);
}

void FileTreeBridge::toggleSelect(const std::string &nodePath) {
  fileTreeController->toggle_select_for_path(nodePath);
}

void FileTreeBridge::setCollapsed(const std::string &directoryPath) {
  fileTreeController->set_collapsed(directoryPath);
}

void FileTreeBridge::refreshDirectory(const std::string &directoryPath) {
  fileTreeController->refresh_dir(directoryPath);
}

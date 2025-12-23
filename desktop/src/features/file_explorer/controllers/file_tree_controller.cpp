#include "file_tree_controller.h"

FileTreeController::FileTreeController(neko::FileTree *fileTree,
                                       QObject *parent)
    : QObject(parent), fileTree(fileTree) {}

neko::FileTreeSnapshot FileTreeController::getTreeSnapshot() {
  return fileTree->get_tree_snapshot();
}

std::string FileTreeController::getParentNodePath(const std::string &path) {
  return fileTree->get_path_of_parent(path).c_str();
}

std::vector<neko::FileNodeSnapshot>
FileTreeController::getVisibleChildren(const std::string &directoryPath) {
  std::vector<neko::FileNodeSnapshot> children = {};
  rust::Vec<neko::FileNodeSnapshot> rawChildren =
      fileTree->get_children(directoryPath);

  if (!rawChildren.empty()) {
    for (const auto &child : rawChildren) {
      children.push_back(child);
    }
  }

  return children;
}

neko::FileNodeSnapshot
FileTreeController::getPreviousNode(const std::string &currentNodePath) {
  return fileTree->get_prev_node(currentNodePath);
}

neko::FileNodeSnapshot
FileTreeController::getNextNode(const std::string &currentNodePath) {
  return fileTree->get_next_node(currentNodePath);
}

void FileTreeController::setRootDir(const std::string &path) {
  fileTree->set_root_dir(path);
}

void FileTreeController::setExpanded(const std::string &directoryPath) {
  fileTree->set_expanded(directoryPath);
}

void FileTreeController::setCurrent(const std::string &itemPath) {
  fileTree->set_current(itemPath);
}

void FileTreeController::clearCurrent() { fileTree->clear_current(); }

void FileTreeController::toggleExpanded(const std::string &directoryPath) {
  fileTree->toggle_expanded(directoryPath);
}

void FileTreeController::toggleSelect(const std::string &nodePath) {
  fileTree->toggle_select(nodePath);
}

void FileTreeController::setCollapsed(const std::string &directoryPath) {
  fileTree->set_collapsed(directoryPath);
}

void FileTreeController::refreshDirectory(const std::string &directoryPath) {
  fileTree->refresh_dir(directoryPath);
}

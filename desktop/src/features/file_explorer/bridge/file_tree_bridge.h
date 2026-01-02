#ifndef FILE_TREE_BRIDGE_H
#define FILE_TREE_BRIDGE_H

#include "neko-core/src/ffi/bridge.rs.h"
#include <QObject>

class FileTreeBridge : public QObject {
  Q_OBJECT

public:
  struct FileTreeBridgeProps {
    rust::Box<neko::FileTreeController> fileTreeController;
  };

  explicit FileTreeBridge(FileTreeBridgeProps props, QObject *parent = nullptr);
  ~FileTreeBridge() override = default;

  neko::FileTreeSnapshot getTreeSnapshot();
  std::string getParentNodePath(const std::string &path);
  std::vector<neko::FileNodeSnapshot>
  getVisibleChildren(const std::string &directoryPath);
  neko::FileNodeSnapshot getPreviousNode(const std::string &currentNodePath);
  neko::FileNodeSnapshot getNextNode(const std::string &currentNodePath);

  void setRootDir(const std::string &path);
  void setExpanded(const std::string &directoryPath);
  void setCurrent(const std::string &itemPath);
  void clearCurrent();
  void toggleExpanded(const std::string &directoryPath);
  void toggleSelect(const std::string &nodePath);
  void setCollapsed(const std::string &directoryPath);
  void refreshDirectory(const std::string &directoryPath);

private:
  rust::Box<neko::FileTreeController> fileTreeController;
};

#endif

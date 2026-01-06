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
  QString getParentNodePath(const QString &path);
  std::vector<neko::FileNodeSnapshot>
  getVisibleChildren(const QString &directoryPath);
  neko::FileNodeSnapshot getPreviousNode(const QString &currentNodePath);
  neko::FileNodeSnapshot getNextNode(const QString &currentNodePath);

  void setRootDirectory(const QString &rootDirectoryPath);
  void setExpanded(const QString &directoryPath);
  void setCurrent(const QString &itemPath);
  void clearCurrent();
  void toggleExpanded(const QString &directoryPath);
  void toggleSelect(const QString &nodePath);
  void setCollapsed(const QString &directoryPath);
  void refreshDirectory(const QString &directoryPath);

private:
  rust::Box<neko::FileTreeController> fileTreeController;
};

#endif

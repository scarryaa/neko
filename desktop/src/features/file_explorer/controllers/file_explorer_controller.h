#ifndef FILE_EXPLORER_CONTROLLER_H
#define FILE_EXPLORER_CONTROLLER_H

#include "features/file_explorer/bridge/file_tree_bridge.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class FileTreeBridge;

/// \class FileExplorerController
/// \brief Handles high-level `FileExplorerWidget` operations.
///
/// The purpose of FileExplorerController is to handle various file explorer
/// operations -- e.g. cut/copy, loading a directory, etc. -- so the file
/// explorer widget can focus on UI-related concerns, and remain unaware of the
/// specifics of the previously mentioned operations.
///
/// FileExplorerController differs from `FileTreeBridge` in that rather than
/// performing the "raw" Rust call for an operation, it first calls into Rust
/// via FileTreeBridge, and then uses the result of that call as needed and
/// performs any necessary bookkeeping afterwards.
class FileExplorerController : QObject {
  Q_OBJECT

public:
  struct FileExplorerControllerProps {
    FileTreeBridge *fileTreeBridge;
  };

  struct FileNodeInfo {
    int index = -1;
    neko::FileNodeSnapshot nodeSnapshot;

    [[nodiscard]] bool foundNode() const { return index != -1; }
  };

  explicit FileExplorerController(const FileExplorerControllerProps &props,
                                  QObject *parent = nullptr);
  ~FileExplorerController() override = default;

  template <typename Predicate>
  FileExplorerController::FileNodeInfo getNode(Predicate predicate) {
    auto snapshot = fileTreeBridge->getTreeSnapshot();
    int index = 0;

    for (const auto &node : snapshot.nodes) {
      if (predicate(node)) {
        return {.index = index, .nodeSnapshot = node};
      }

      index++;
    }

    return {};
  }

  FileExplorerController::FileNodeInfo getNodeByIndex(int targetIndex) {
    auto snapshot = fileTreeBridge->getTreeSnapshot();
    int index = 0;

    for (const auto &node : snapshot.nodes) {
      if (targetIndex == index) {
        return {.index = index, .nodeSnapshot = node};
      }

      index++;
    }

    return {};
  }

  neko::FileTreeSnapshot getTreeSnapshot();
  int getNodeCount();

  void loadDirectory(const QString &rootDirectoryPath);
  void setExpanded(const QString &directoryPath);
  void setCurrent(const QString &targetPath);

  void handleCut();
  void handleCopy();

  neko::FileExplorerContextFfi getCurrentContext();

signals:
  void rootDirectoryChanged(const QString &rootDirectoryPath);

private:
  FileTreeBridge *fileTreeBridge;
};

#endif

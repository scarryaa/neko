#ifndef FILE_EXPLORER_CONTROLLER_H
#define FILE_EXPLORER_CONTROLLER_H

#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/file_explorer/types/types.h"
#include <QObject>
#include <functional>
#include <neko-core/src/ffi/bridge.rs.h>
#include <unordered_map>

namespace std {
template <> struct hash<QKeyCombination> {
  size_t operator()(const QKeyCombination &combo) const noexcept {
    return qHash(combo);
  }
};
} // namespace std

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
class FileExplorerController : public QObject {
  Q_OBJECT

public:
  struct FileExplorerControllerProps {
    FileTreeBridge *fileTreeBridge;
  };

  explicit FileExplorerController(const FileExplorerControllerProps &props,
                                  QObject *parent = nullptr);
  ~FileExplorerController() override = default;

  template <typename Predicate> FileNodeInfo getNode(Predicate predicate) {
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

  FileNodeInfo getNodeByIndex(int targetIndex) {
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

  ChangeSet handleKeyPress(int key, Qt::KeyboardModifiers modifiers);
  FileNodeInfo handleNodeClick(int row, bool wasLeftMouseButton);
  void handleNodeClickRelease(int row, bool wasLeftMouseButton);
  void handleNodeDoubleClick(int row, bool wasLeftMouseButton);
  void handleNodeDrop(int row, const QByteArray &encodedData);

  void triggerCommand(
      const std::string &commandId, bool bypassDeleteConfirmation = false,
      int index = -1, const TargetNodePath &targetNodePath = TargetNodePath{""},
      const DestinationNodePath &destinationNodePath = DestinationNodePath{""});

  void handleCut();
  void handleCopy();

  neko::FileExplorerContextFfi getCurrentContext();

signals:
  void rootDirectoryChanged(const QString &rootDirectoryPath);
  void commandRequested(const std::string &commandId,
                        const neko::FileExplorerContextFfi &ctx,
                        bool bypassDeleteConfirmation);
  void requestFocusEditor(bool shouldFocus);

private:
  void bind(QKeyCombination combo, std::function<void()> action);

  bool doubleClickPending = false;
  bool needsScroll = false;
  FontSizeAdjustment fontSizeAdjustment = FontSizeAdjustment::NoChange;
  std::unordered_map<QKeyCombination, std::function<void()>> keyMappings;
  FileTreeBridge *fileTreeBridge;
};

#endif

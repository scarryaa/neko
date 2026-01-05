#ifndef FILE_EXPLORER_FLOWS_H
#define FILE_EXPLORER_FLOWS_H

#include "core/bridge/app_bridge.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/main_window/ui_handles.h"
#include <vector>

/// \class FileExplorerFlows
/// \brief Orchestrates file explorer-related workflows that involve multiple
/// controllers and UI pieces (dialogs, status bar, tab bar, etc.). It is
/// internal to WorkspaceCoordinator.
class FileExplorerFlows {
public:
  struct FileExplorerFlowsProps {
    AppBridge *appBridge;
    FileTreeBridge *fileTreeBridge;
    const UiHandles uiHandles;
  };

  struct FileExplorerFlowsCommandResult {
    bool success;
    bool shouldRedraw;
    std::vector<neko::FileExplorerUiIntentKindFfi> intentKinds;
  };

  explicit FileExplorerFlows(const FileExplorerFlowsProps &props);
  ~FileExplorerFlows() = default;

  FileExplorerFlowsCommandResult
  handleFileExplorerCommand(const std::string &commandId,
                            const neko::FileExplorerContextFfi &ctx,
                            bool bypassDeleteConfirmation);

private:
  static void handleCut(const QString &itemPath);
  static void handleCopy(const QString &itemPath);
  bool handleDuplicate(const QString &itemPath, const QString &parentItemPath);
  bool handlePaste(const QString &itemPath, const QString &parentItemPath);
  static void handleCopyPath(const QString &itemPath);
  void handleCopyRelativePath(const QString &itemPath);

  FileTreeBridge *fileTreeBridge;
  AppBridge *appBridge;
  const UiHandles uiHandles;
};

#endif // FILE_EXPLORER_FLOWS_H

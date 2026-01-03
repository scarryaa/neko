#ifndef FILE_EXPLORER_FLOWS_H
#define FILE_EXPLORER_FLOWS_H

#include "core/bridge/app_bridge.h"
#include "features/main_window/ui_handles.h"

/// \class FileExplorerFlows
/// \brief Orchestrates file explorer-related workflows that involve multiple
/// controllers and UI pieces (dialogs, status bar, tab bar, etc.). It is
/// internal to WorkspaceCoordinator.
class FileExplorerFlows {
public:
  struct FileExplorerFlowsProps {
    AppBridge *appBridge;
    const UiHandles uiHandles;
  };

  explicit FileExplorerFlows(const FileExplorerFlowsProps &props);
  ~FileExplorerFlows() = default;

  bool handleFileExplorerCommand(const std::string &commandId,
                                 const neko::FileExplorerContextFfi &ctx);

private:
  AppBridge *appBridge;
  const UiHandles uiHandles;
};

#endif // FILE_EXPLORER_FLOWS_H

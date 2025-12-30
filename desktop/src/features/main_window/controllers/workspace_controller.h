#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

class TabController;
class AppStateController;

#include "features/main_window/interfaces/workspace_ui.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <functional>

class WorkspaceController {
public:
  struct WorkspaceControllerProps {
    TabController *tabController;
    AppStateController *appStateController;
    WorkspaceUi workspaceUi;
  };

  explicit WorkspaceController(const WorkspaceControllerProps &props);
  ~WorkspaceController() = default;

  QList<int> closeTabs(neko::CloseTabOperationTypeFfi operationType, int tabId,
                       bool forceClose);

  neko::FileOpenResult openFile(const QString &startingPath);
  bool saveTab(int tabId, bool forceSaveAs);
  bool saveTabWithPromptIfNeeded(int tabId, bool isSaveAs);
  [[nodiscard]] std::optional<std::string> requestFileExplorerDirectory() const;

private:
  bool closeMany(const QList<int> &ids, bool forceClose,
                 const std::function<void()> &closeAction);
  static neko::FileOpenResult makeFailedOpenResult();

  AppStateController *appStateController;
  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

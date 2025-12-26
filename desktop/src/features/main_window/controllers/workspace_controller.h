#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

class TabController;

#include "features/main_window/interfaces/workspace_ui.h"
#include <QList>
#include <functional>

class WorkspaceController {
public:
  struct WorkspaceControllerProps {
    TabController *tabController;
    WorkspaceUi workspaceUi;
  };

  explicit WorkspaceController(const WorkspaceControllerProps &props);
  ~WorkspaceController() = default;

  QList<int> closeLeft(int tabId, bool forceClose);
  QList<int> closeRight(int tabId, bool forceClose);
  QList<int> closeOthers(int tabId, bool forceClose);
  QList<int> closeTab(int tabId, bool forceClose);

  bool saveTab(int tabId, bool forceSaveAs);
  bool saveTabWithPromptIfNeeded(int tabId, bool isSaveAs);
  [[nodiscard]] std::optional<std::string> requestFileExplorerDirectory() const;

private:
  bool closeMany(const QList<int> &ids, bool forceClose,
                 const std::function<void()> &closeAction);

  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

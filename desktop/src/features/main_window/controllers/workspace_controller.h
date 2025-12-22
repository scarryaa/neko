#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

#include "features/main_window/hooks/workspace_ui.h"
#include "features/tabs/controllers/tab_controller.h"

class WorkspaceController {
public:
  WorkspaceController(TabController *tabController, WorkspaceUi workspaceUi);
  ~WorkspaceController();

  const QList<int> closeLeft(int id, bool forceClose);
  const QList<int> closeRight(int id, bool forceClose);
  const QList<int> closeOthers(int id, bool forceClose);
  const QList<int> closeTab(int id, bool forceClose);

  bool saveTab(int id, bool isSaveAs);
  bool saveTabWithPromptIfNeeded(int id, bool isSaveAs);

private:
  bool closeMany(const QList<int> &ids, bool forceClose,
                 std::function<void()> closeAction);

  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

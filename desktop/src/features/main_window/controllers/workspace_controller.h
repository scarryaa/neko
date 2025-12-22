#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

#include "features/main_window/hooks/workspace_ui.h"
#include "features/tabs/controllers/tab_controller.h"

class WorkspaceController {
public:
  WorkspaceController(TabController *tabController, WorkspaceUi workspaceUi);
  ~WorkspaceController();

  void closeLeft(int id, bool forceClose);
  void closeRight(int id, bool forceClose);
  void closeOthers(int id, bool forceClose);
  void closeTab(int id, bool forceClose);

  bool saveTabWithPromptIfNeeded(int id);

private:
  void closeMany(const QList<int> &ids, bool forceClose,
                 std::function<void()> closeAction);

  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

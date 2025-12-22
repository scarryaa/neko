#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

#include "features/main_window/hooks/workspace_ui.h"
#include "features/tabs/controllers/tab_controller.h"

class WorkspaceController {
public:
  WorkspaceController(TabController *tabController, WorkspaceUi workspaceUi);
  ~WorkspaceController();

  void closeMany(const QList<int> &ids, bool forceClose,
                 std::function<void()> closeAction);
  bool saveTabWithPromptIfNeeded(int id);

private:
  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

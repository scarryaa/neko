#ifndef WORKSPACE_CONTROLLER_H
#define WORKSPACE_CONTROLLER_H

#include "features/main_window/hooks/workspace_ui.h"
#include "features/tabs/controllers/tab_controller.h"
#include <QHash>

class WorkspaceController {
public:
  WorkspaceController(TabController *tabController, WorkspaceUi workspaceUi);
  ~WorkspaceController() = default;

  QList<int> closeLeft(int tabId, bool forceClose);
  QList<int> closeRight(int tabId, bool forceClose);
  QList<int> closeOthers(int tabId, bool forceClose);
  QList<int> closeTab(int tabId, bool forceClose);

  bool saveTab(int tabId, bool forceSaveAs);
  bool saveTabWithPromptIfNeeded(int tabId, bool isSaveAs);

private:
  bool closeMany(const QList<int> &ids, bool forceClose,
                 const std::function<void()> &closeAction);

  TabController *tabController;
  WorkspaceUi workspaceUi;
};

#endif

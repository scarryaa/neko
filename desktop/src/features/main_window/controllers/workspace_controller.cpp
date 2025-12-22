#include "workspace_controller.h"

WorkspaceController::WorkspaceController(TabController *tabController,
                                         WorkspaceUi workspaceUi)
    : tabController(tabController), workspaceUi(workspaceUi) {}

WorkspaceController::~WorkspaceController() {}

void WorkspaceController::closeLeft(int id, bool forceClose) {
  auto ids = tabController->getCloseLeftTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeLeftTabs(id); });
}

void WorkspaceController::closeRight(int id, bool forceClose) {
  auto ids = tabController->getCloseRightTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeRightTabs(id); });
}

void WorkspaceController::closeOthers(int id, bool forceClose) {
  auto ids = tabController->getCloseOtherTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeOtherTabs(id); });
}

void WorkspaceController::closeTab(int id, bool forceClose) {
  auto ids = QList<int>();
  ids.push_back(id);

  closeMany(ids, forceClose, [this, id]() { tabController->closeTab(id); });
}

void WorkspaceController::closeMany(const QList<int> &ids, bool forceClose,
                                    std::function<void()> closeAction) {
  if (ids.isEmpty())
    return;

  bool anyModified = false;
  for (int id : ids) {
    if (tabController->getTabModified(id)) {
      anyModified = true;
      break;
    }
  }

  if (!forceClose && anyModified) {
    switch (workspaceUi.confirmCloseTabs(ids)) {
    case CloseDecision::Save:
      for (int id : ids) {
        if (tabController->getTabModified(id)) {
          if (!saveTabWithPromptIfNeeded(id))
            return;
        }
      }
      break;
    case CloseDecision::DontSave:
      break;
    case CloseDecision::Cancel:
      return;
    }
  }

  closeAction();
}

bool WorkspaceController::saveTabWithPromptIfNeeded(int id) {
  const QString path = tabController->getTabPath(id);

  if (!path.isEmpty()) {
    return tabController->saveTabWithId(id);
  }

  // TODO: Fill in file name/path if available
  // TODO: Switch to relevant tab
  const QString filePath = workspaceUi.promptSaveAsPath(id);
  if (filePath.isEmpty())
    return false;

  return tabController->saveTabWithIdAndSetPath(id, filePath.toStdString());
}

#include "workspace_controller.h"

WorkspaceController::WorkspaceController(TabController *tabController,
                                         WorkspaceUi workspaceUi)
    : tabController(tabController), workspaceUi(workspaceUi) {}

WorkspaceController::~WorkspaceController() {}

const QList<int> WorkspaceController::closeLeft(int id, bool forceClose) {
  auto ids = tabController->getCloseLeftTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeLeftTabs(id); });

  return ids;
}

const QList<int> WorkspaceController::closeRight(int id, bool forceClose) {
  auto ids = tabController->getCloseRightTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeRightTabs(id); });

  return ids;
}

const QList<int> WorkspaceController::closeOthers(int id, bool forceClose) {
  auto ids = tabController->getCloseOtherTabIds(id);
  closeMany(ids, forceClose,
            [this, id]() { tabController->closeOtherTabs(id); });

  return ids;
}

const QList<int> WorkspaceController::closeTab(int id, bool forceClose) {
  auto ids = QList<int>();
  ids.push_back(id);

  closeMany(ids, forceClose, [this, id]() { tabController->closeTab(id); });

  return ids;
}

bool WorkspaceController::saveTab(int id, bool forceSaveAs) {
  return saveTabWithPromptIfNeeded(id, forceSaveAs);
}

bool WorkspaceController::saveTabWithPromptIfNeeded(int id, bool isSaveAs) {
  const QString path = tabController->getTabPath(id);

  if (!path.isEmpty() && !isSaveAs) {
    return tabController->saveTabWithId(id);
  }

  // TODO: Fill in file name/path if available
  // TODO: Switch to relevant tab
  const QString filePath = workspaceUi.promptSaveAsPath(id);
  if (filePath.isEmpty())
    return false;

  return tabController->saveTabWithIdAndSetPath(id, filePath.toStdString());
}

bool WorkspaceController::closeMany(const QList<int> &ids, bool forceClose,
                                    std::function<void()> closeAction) {
  if (ids.isEmpty())
    return false;

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
          if (!saveTabWithPromptIfNeeded(id, false))
            return false;
        }
      }
      break;
    case CloseDecision::DontSave:
      break;
    case CloseDecision::Cancel:
      return false;
    }
  }

  closeAction();
  return true;
}

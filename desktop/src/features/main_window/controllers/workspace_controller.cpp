#include "workspace_controller.h"
#include "features/tabs/controllers/tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QHash>
#include <QString>

WorkspaceController::WorkspaceController(const WorkspaceControllerProps &props)
    : tabController(props.tabController), workspaceUi(props.workspaceUi) {}

QList<int> WorkspaceController::closeLeft(int tabId, bool forceClose) {
  auto ids = tabController->getCloseLeftTabIds(tabId);
  closeMany(ids, forceClose,
            [this, tabId]() { tabController->closeLeftTabs(tabId); });

  return ids;
}

QList<int> WorkspaceController::closeRight(int tabId, bool forceClose) {
  auto ids = tabController->getCloseRightTabIds(tabId);
  closeMany(ids, forceClose,
            [this, tabId]() { tabController->closeRightTabs(tabId); });

  return ids;
}

QList<int> WorkspaceController::closeOthers(int tabId, bool forceClose) {
  auto ids = tabController->getCloseOtherTabIds(tabId);
  closeMany(ids, forceClose,
            [this, tabId]() { tabController->closeOtherTabs(tabId); });

  return ids;
}

QList<int> WorkspaceController::closeTab(int tabId, bool forceClose) {
  auto ids = QList<int>();
  ids.push_back(tabId);

  closeMany(ids, forceClose,
            [this, tabId]() { tabController->closeTab(tabId); });

  return ids;
}

bool WorkspaceController::saveTab(int tabId, bool forceSaveAs) {
  return saveTabWithPromptIfNeeded(tabId, forceSaveAs);
}

bool WorkspaceController::saveTabWithPromptIfNeeded(int tabId, bool isSaveAs) {
  const auto snapshot = tabController->getTabsSnapshot();
  QString path = QString();
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
    }
  }

  if (!path.isEmpty() && !isSaveAs) {
    return tabController->saveTabWithId(tabId);
  }

  // TODO(scarlet): Fill in file name/path if available
  // TODO(scarlet): Switch to relevant tab
  const QString filePath = workspaceUi.promptSaveAsPath(tabId);
  if (filePath.isEmpty()) {
    return false;
  }

  return tabController->saveTabWithIdAndSetPath(tabId, filePath.toStdString());
}

bool WorkspaceController::closeMany(const QList<int> &ids, bool forceClose,
                                    const std::function<void()> &closeAction) {
  if (ids.isEmpty()) {
    return false;
  }

  const auto snap = tabController->getTabsSnapshot();

  QHash<int, bool> modifiedById = QHash<int, bool>();
  modifiedById.reserve(static_cast<int>(snap.tabs.size()));
  for (const auto &tab : snap.tabs) {
    modifiedById.insert(static_cast<int>(tab.id), tab.modified);
  }

  auto isModified = [&](int tabId) -> bool {
    auto foundTab = modifiedById.constFind(tabId);
    return foundTab != modifiedById.constEnd() ? foundTab.value() : false;
  };

  bool anyModified = false;
  for (int tabId : ids) {
    if (isModified(tabId)) {
      anyModified = true;
      break;
    }
  }

  if (!forceClose && anyModified) {
    switch (workspaceUi.confirmCloseTabs(ids)) {
    case CloseDecision::Save:
      for (int tabId : ids) {
        if (isModified(tabId)) {
          if (!saveTabWithPromptIfNeeded(tabId, false)) {
            return false;
          }
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

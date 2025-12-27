#include "workspace_controller.h"
#include "features/tabs/controllers/tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QFileInfo>
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

QList<int> WorkspaceController::closeAll(bool forceClose) {
  auto ids = tabController->getCloseAllTabIds();
  closeMany(ids, forceClose, [this]() { tabController->closeAllTabs(); });

  return ids;
}

QList<int> WorkspaceController::closeClean() {
  auto ids = tabController->getCloseCleanTabIds();
  closeMany(ids, false, [this]() { tabController->closeCleanTabs(); });

  return ids;
}

// TODO(scarlet): Refocus editor on tab click
QList<int> WorkspaceController::closeTab(int tabId, bool forceClose) {
  auto ids = QList<int>();
  ids.push_back(tabId);

  closeMany(ids, forceClose,
            [this, tabId]() { tabController->closeTab(tabId); });

  return ids;
}

bool WorkspaceController::openFile(const QString &startingPath) {
  QString initialDir;

  if (!startingPath.isEmpty()) {
    QFileInfo info(startingPath);
    initialDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  }

  const QString filePath = workspaceUi.openFile(initialDir);
  if (filePath.isEmpty()) {
    return false;
  }

  const auto snapshot = tabController->getTabsSnapshot();
  auto targetTabId = tabController->addTab();

  return tabController->openFile(targetTabId, filePath.toStdString());
}

bool WorkspaceController::saveTab(int tabId, bool forceSaveAs) {
  return saveTabWithPromptIfNeeded(tabId, forceSaveAs);
}

bool WorkspaceController::saveTabWithPromptIfNeeded(int tabId, bool isSaveAs) {
  const auto snapshot = tabController->getTabsSnapshot();
  QString fileName;
  QString path;

  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
      fileName = QString::fromUtf8(tab.title);
      break;
    }
  }

  if (!path.isEmpty() && !isSaveAs) {
    return tabController->saveTabWithId(tabId);
  }

  QString initialDir;
  if (!path.isEmpty()) {
    QFileInfo info(path);

    if (info.isDir()) {
      initialDir = info.absoluteFilePath();
    } else {
      initialDir = info.absolutePath();
    }
  }

  const QString filePath = workspaceUi.promptSaveAsPath(initialDir, fileName);

  if (filePath.isEmpty()) {
    return false;
  }

  return tabController->saveTabWithIdAndSetPath(tabId, filePath.toStdString());
}

std::optional<std::string>
WorkspaceController::requestFileExplorerDirectory() const {
  const QString dir = workspaceUi.promptFileExplorerDirectory();
  if (dir.isEmpty()) {
    return std::nullopt;
  }

  return dir.toStdString();
}

bool WorkspaceController::closeMany(const QList<int> &ids, bool forceClose,
                                    const std::function<void()> &closeAction) {
  if (ids.isEmpty()) {
    return false;
  }

  const auto snapshot = tabController->getTabsSnapshot();

  QHash<int, bool> modifiedById;
  modifiedById.reserve(static_cast<int>(snapshot.tabs.size()));
  for (const auto &tab : snapshot.tabs) {
    modifiedById.insert(static_cast<int>(tab.id), tab.modified);
  }

  auto isModified = [&](int tabId) -> bool {
    auto maybeModifiedTab = modifiedById.constFind(tabId);
    return maybeModifiedTab != modifiedById.constEnd()
               ? maybeModifiedTab.value()
               : false;
  };

  QList<int> modifiedIds;
  modifiedIds.reserve(ids.size());
  for (int tabId : ids) {
    if (isModified(tabId)) {
      modifiedIds.append(tabId);
    }
  }

  if (!forceClose && !modifiedIds.isEmpty()) {
    if (modifiedIds.size() == 1) {
      workspaceUi.focusTab(modifiedIds.first());
    }

    switch (workspaceUi.confirmCloseTabs(ids)) {
    case CloseDecision::Save:
      for (int tabId : modifiedIds) {
        workspaceUi.focusTab(tabId);

        if (!saveTabWithPromptIfNeeded(tabId, false)) {
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

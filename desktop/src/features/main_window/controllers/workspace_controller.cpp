#include "workspace_controller.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/tabs/controllers/tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QFileInfo>
#include <QHash>
#include <QString>

WorkspaceController::WorkspaceController(const WorkspaceControllerProps &props)
    : tabController(props.tabController),
      appStateController(props.appStateController),
      workspaceUi(props.workspaceUi) {}

neko::FileOpenResult WorkspaceController::makeFailedOpenResult() {
  neko::FileOpenResult result{};
  result.success = false;
  result.snapshot = {};

  return result;
}

// TODO(scarlet): Refocus editor on tab click
QList<int>
WorkspaceController::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                               int tabId, bool forceClose) {
  const bool closePinned =
      (operationType == neko::CloseTabOperationTypeFfi::Single) && forceClose;
  auto idsToClose =
      tabController->getCloseTabIds(operationType, tabId, closePinned);

  closeMany(idsToClose, forceClose,
            [this, tabId, operationType, closePinned]() {
              tabController->closeTabs(operationType, tabId, closePinned);
            });

  return idsToClose;
}

neko::FileOpenResult
WorkspaceController::openFile(const QString &startingPath) {
  QString initialDir;

  if (!startingPath.isEmpty()) {
    QFileInfo info(startingPath);
    initialDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  }

  const QString filePath = workspaceUi.openFile(initialDir);
  if (filePath.isEmpty()) {
    return makeFailedOpenResult();
  }

  auto targetTabId = tabController->addTab();
  return appStateController->openFile(targetTabId, filePath.toStdString());
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
    return appStateController->saveTab(tabId);
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

  return appStateController->saveTabAs(tabId, filePath.toStdString());
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

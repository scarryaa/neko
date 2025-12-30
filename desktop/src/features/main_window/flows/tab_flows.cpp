#include "tab_flows.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_controller.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QHash>
#include <QMessageBox>
#include <QScrollBar>
#include <QSet>
#include <QString>
#include <neko-core/src/ffi/bridge.rs.h>

TabFlows::TabFlows(const TabFlowsProps &props)
    : tabController(props.tabController), appController(props.appController),
      editorController(props.editorController), uiHandles(props.uiHandles) {}

void TabFlows::handleTabCommand(const std::string &commandId,
                                const neko::TabContextFfi &ctx,
                                bool forceClose) {
  if (commandId.empty()) {
    return;
  }

  const int tabId = static_cast<int>(ctx.id);

  // TODO(scarlet): Avoid matching on hardcoded command ids
  if (commandId == "tab.close") {
    closeTabs(neko::CloseTabOperationTypeFfi::Single, tabId, forceClose);
  } else if (commandId == "tab.closeOthers") {
    closeTabs(neko::CloseTabOperationTypeFfi::Others, tabId, forceClose);
  } else if (commandId == "tab.closeLeft") {
    closeTabs(neko::CloseTabOperationTypeFfi::Left, tabId, forceClose);
  } else if (commandId == "tab.closeRight") {
    closeTabs(neko::CloseTabOperationTypeFfi::Right, tabId, forceClose);
  } else if (commandId == "tab.closeAll") {
    closeTabs(neko::CloseTabOperationTypeFfi::All, 0, forceClose);
  } else if (commandId == "tab.closeClean") {
    closeTabs(neko::CloseTabOperationTypeFfi::Clean, 0, forceClose);
  } else if (commandId == "tab.copyPath") {
    copyTabPath(tabId);
  } else if (commandId == "tab.reveal") {
    revealTab(ctx);
  } else if (commandId == "tab.pin") {
    tabTogglePin(tabId, ctx.is_pinned);
  }
}

void TabFlows::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                         int anchorTabId, bool forceClose) {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    // Close the window if there are no tabs
    QApplication::quit();
    return;
  }

  saveScrollOffsetsForActiveTab();

  const bool closePinned =
      (operationType == neko::CloseTabOperationTypeFfi::Single) && forceClose;

  auto ids =
      tabController->getCloseTabIds(operationType, anchorTabId, closePinned);

  closeManyTabs(
      ids, forceClose, [this, anchorTabId, operationType, closePinned]() {
        tabController->closeTabs(operationType, anchorTabId, closePinned);
      });
}

void TabFlows::fileSaved(bool saveAs) {
  const auto snapshot = tabController->getTabsSnapshot();

  if (!snapshot.active_present) {
    return;
  }

  const int activeId = static_cast<int>(snapshot.active_id);
  const bool success = saveTabWithPromptIfNeeded(activeId, saveAs);

  if (success) {
    uiHandles.tabBarWidget->setTabModified(activeId, false);
    tabController->tabSaved(activeId);
  }
}

void TabFlows::tabTogglePin(int tabId, bool isPinned) {
  if (isPinned) {
    tabController->unpinTab(tabId);
  } else {
    tabController->pinTab(tabId);
  }
}

void TabFlows::copyTabPath(int tabId) {
  const auto snapshot = tabController->getTabsSnapshot();
  QString path;

  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
      break;
    }
  }

  if (path.isEmpty()) {
    return;
  }

  QApplication::clipboard()->setText(path);
}

bool TabFlows::revealTab(const neko::TabContextFfi &ctx) {
  if (!ctx.file_path_present) {
    return false;
  }

  appController->runTabCommand("tab.reveal", ctx, false);
  return true;
}

void TabFlows::newTab() {
  saveScrollOffsetsForActiveTab();
  tabController->addTab();
}

void TabFlows::tabChanged(int tabId) {
  saveScrollOffsetsForActiveTab();
  tabController->setActiveTab(tabId);
}

void TabFlows::tabUnpinned(int tabId) { tabController->unpinTab(tabId); }

void TabFlows::moveTabBy(int delta, bool useHistory) {
  tabController->moveTabBy(delta, useHistory);
}

void TabFlows::bufferChanged() {
  const auto snapshot = tabController->getTabsSnapshot();
  const int activeId = static_cast<int>(snapshot.active_id);

  bool modified = false;
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == activeId) {
      modified = tab.modified;
      break;
    }
  }

  uiHandles.tabBarWidget->setTabModified(activeId, modified);
}

void TabFlows::handleTabsClosed() {
  const auto snapshot = tabController->getTabsSnapshot();
  const int newTabCount = static_cast<int>(snapshot.tabs.size());

  uiHandles.statusBarWidget->onTabClosed(newTabCount);
}

// TODO(scarlet): Move this to TabController?
int TabFlows::getModifiedTabCount(const QList<int> &ids) {
  const auto snapshot = tabController->getTabsSnapshot();

  QSet<int> idSet;
  idSet.reserve(ids.size());
  for (int tabId : ids) {
    idSet.insert(tabId);
  }

  int modifiedCount = 0;
  for (const auto &tab : snapshot.tabs) {
    const int tabId = static_cast<int>(tab.id);
    if (idSet.contains(tabId) && tab.modified) {
      modifiedCount++;
    }
  }

  return modifiedCount;
}

SaveResult TabFlows::saveTab(int tabId, bool isSaveAs) {
  if (saveTabWithPromptIfNeeded(tabId, isSaveAs)) {
    uiHandles.tabBarWidget->setTabModified(tabId, false);
    return SaveResult::Saved;
  }

  qInfo() << "Save as failed";
  return SaveResult::Failed;
}

bool TabFlows::saveTabWithPromptIfNeeded(int tabId, bool isSaveAs) {
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
    return appController->saveTab(tabId);
  }

  QString initialDir;
  if (!path.isEmpty()) {
    QFileInfo info(path);
    initialDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  }

  const QString filePath =
      DialogService::openSaveAsDialog(initialDir, fileName, uiHandles.window);
  if (filePath.isEmpty()) {
    return false;
  }

  return appController->saveTabAs(tabId, filePath.toStdString());
}

bool TabFlows::closeManyTabs(const QList<int> &ids, bool forceClose,
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
      tabChanged(modifiedIds.first());
    }

    int modifiedCount = getModifiedTabCount(ids);
    switch (DialogService::openCloseConfirmationDialog(ids, modifiedCount,
                                                       uiHandles.window)) {
    case CloseDecision::Save:
      for (int tabId : modifiedIds) {
        tabChanged(modifiedIds.first());

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
  handleTabsClosed();
  return true;
}

void TabFlows::saveScrollOffsetsForActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();

  if (!snapshot.active_present) {
    return;
  }

  const int activeId = static_cast<int>(snapshot.active_id);
  const double horizontalScrollOffset =
      uiHandles.editorWidget->horizontalScrollBar()->value();
  const double verticalScrollOffset =
      uiHandles.editorWidget->verticalScrollBar()->value();

  neko::ScrollOffsetFfi scrollOffsets{
      static_cast<int32_t>(horizontalScrollOffset),
      static_cast<int32_t>(verticalScrollOffset)};

  tabController->setTabScrollOffsets(activeId, scrollOffsets);
}

void TabFlows::restoreScrollOffsetsForActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();

  neko::ScrollOffsetFfi offsets;
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == snapshot.active_id) {
      offsets = tab.scroll_offsets;
      break;
    }
  }

  uiHandles.editorWidget->horizontalScrollBar()->setValue(offsets.x);
  uiHandles.editorWidget->verticalScrollBar()->setValue(offsets.y);
}

void TabFlows::restoreScrollOffsetsForReopenedTab(
    const TabScrollOffsets &scrollOffsets) const {
  uiHandles.editorWidget->horizontalScrollBar()->setValue(
      static_cast<int>(scrollOffsets.x));
  uiHandles.editorWidget->verticalScrollBar()->setValue(
      static_cast<int>(scrollOffsets.y));
}

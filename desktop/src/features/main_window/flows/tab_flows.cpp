#include "tab_flows.h"
#include "core/bridge/app_bridge.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/bridge/tab_bridge.h"
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
    : tabBridge(props.tabBridge), appBridge(props.appBridge),
      editorBridge(props.editorBridge), uiHandles(props.uiHandles) {}

bool TabFlows::handleTabCommand(const std::string &commandId,
                                const neko::TabContextFfi &ctx,
                                bool forceClose) {
  if (commandId.empty()) {
    return false;
  }

  using OperationType = neko::CloseTabOperationTypeFfi;
  using CommandKind = neko::TabCommandKindFfi;

  // TODO(scarlet): Aim to direct most commands to Rust instead of locally
  // handling them.
  const int tabId = static_cast<int>(ctx.id);
  const auto tabCommand = appBridge->parseCommand<CommandType::Tab>(commandId);
  bool succeeded = false;
  bool isCloseCommand = true;
  OperationType operationType;

  // Switch on the command kind to determine the operation type.
  switch (tabCommand) {
  case CommandKind::Close:
    operationType = OperationType::Single;
    break;
  case CommandKind::CloseOthers:
    operationType = OperationType::Others;
    break;
  case CommandKind::CloseLeft:
    operationType = OperationType::Left;
    break;
  case CommandKind::CloseRight:
    operationType = OperationType::Right;
    break;
  case CommandKind::CloseAll:
    operationType = OperationType::All;
    break;
  case CommandKind::CloseClean:
    operationType = OperationType::Clean;
    break;
  default:
    // It's not a close command, so we handle it separately.
    isCloseCommand = false;
    break;
  }

  if (isCloseCommand) {
    succeeded = closeTabs(operationType, tabId, forceClose);
  }

  // Handle non-close tab commands.
  switch (tabCommand) {
  // TODO(scarlet): Add CopyRelativePath command?
  case neko::TabCommandKindFfi::CopyPath:
    succeeded = copyTabPath(tabId);
    break;
  case neko::TabCommandKindFfi::Reveal:
    succeeded = revealTab(ctx);
    break;
  case neko::TabCommandKindFfi::Pin:
    succeeded = tabTogglePin(tabId, ctx.is_pinned);
    break;
  default:
    // Nothing to do.
    break;
  }

  return succeeded;
}

bool TabFlows::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                         int anchorTabId, bool forceClose) {
  const auto snapshot = tabBridge->getTabsSnapshot();
  if (!snapshot.active_present) {
    // Close the window if there are no tabs.
    QApplication::quit();
    return false;
  }

  saveScrollOffsetsForActiveTab();

  const bool closePinned =
      (operationType == neko::CloseTabOperationTypeFfi::Single) && forceClose;

  auto ids = tabBridge->getCloseTabIds(operationType, anchorTabId, closePinned);

  return closeManyTabs(
      ids, forceClose, [this, anchorTabId, operationType, closePinned]() {
        tabBridge->closeTabs(operationType, anchorTabId, closePinned);
      });
}

void TabFlows::fileSaved(bool saveAs) {
  const auto snapshot = tabBridge->getTabsSnapshot();

  if (!snapshot.active_present) {
    return;
  }

  const int activeId = static_cast<int>(snapshot.active_id);
  const bool success = saveTabWithPromptIfNeeded(activeId, saveAs);

  if (success) {
    uiHandles.tabBarWidget->setTabModified(activeId, false);
    tabBridge->tabSaved(activeId);
  }
}

bool TabFlows::tabTogglePin(int tabId, bool isPinned) {
  bool succeeded = false;
  if (isPinned) {
    succeeded = tabBridge->unpinTab(tabId);
  } else {
    succeeded = tabBridge->pinTab(tabId);
  }

  return succeeded;
}

bool TabFlows::copyTabPath(int tabId) {
  const auto snapshot = tabBridge->getTabsSnapshot();
  QString path;

  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
      break;
    }
  }

  if (path.isEmpty()) {
    return false;
  }

  QApplication::clipboard()->setText(path);
  return true;
}

bool TabFlows::revealTab(const neko::TabContextFfi &ctx) {
  if (!ctx.file_path_present) {
    return false;
  }

  // TODO(scarlet): Create TabCommandResultFfi return type.
  appBridge->runCommand<bool>(CommandType::Tab, "tab.reveal", ctx, false);
  return true;
}

void TabFlows::newTab() {
  saveScrollOffsetsForActiveTab();
  tabBridge->createDocumentTabAndView("Untitled", true, true);
}

void TabFlows::tabChanged(int tabId) {
  saveScrollOffsetsForActiveTab();
  tabBridge->setActiveTab(tabId);
}

void TabFlows::tabUnpinned(int tabId) { tabBridge->unpinTab(tabId); }

void TabFlows::moveTabBy(int delta, bool useHistory) {
  tabBridge->moveTabBy(delta, useHistory);
}

void TabFlows::bufferChanged() {
  const auto snapshot = tabBridge->getTabsSnapshot();
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
  const auto snapshot = tabBridge->getTabsSnapshot();
  const int newTabCount = static_cast<int>(snapshot.tabs.size());

  uiHandles.statusBarWidget->onTabClosed(newTabCount);
}

// TODO(scarlet): Move this to TabBridge?
int TabFlows::getModifiedTabCount(const QList<int> &ids) {
  const auto snapshot = tabBridge->getTabsSnapshot();

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
  const auto snapshot = tabBridge->getTabsSnapshot();
  QString fileName;
  QString path;
  uint64_t documentId;
  bool foundTabId;

  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
      fileName = QString::fromUtf8(tab.title);
      documentId = tab.document_id;
      foundTabId = true;
      break;
    }
  }

  if (!foundTabId) {
    return false;
  }

  if (!path.isEmpty() && !isSaveAs) {
    return appBridge->saveDocument(documentId);
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

  return appBridge->saveDocumentAs(documentId, filePath.toStdString());
}

bool TabFlows::closeManyTabs(const QList<int> &ids, bool forceClose,
                             const std::function<void()> &closeAction) {
  if (ids.isEmpty()) {
    return false;
  }

  const auto snapshot = tabBridge->getTabsSnapshot();

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
  const auto snapshot = tabBridge->getTabsSnapshot();

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

  tabBridge->setTabScrollOffsets(activeId, scrollOffsets);
}

void TabFlows::restoreScrollOffsetsForActiveTab() {
  const auto snapshot = tabBridge->getTabsSnapshot();

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

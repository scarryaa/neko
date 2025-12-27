#include "tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"

// TODO(scarlet): Add tab history preservation (e.g. switch to 'last active' tab
// when closing)
// - Make a setting toggle for switching to 'last active' vs by regular inc/dec
// order on last/next tab shortcut?
TabPresentation TabController::fromSnapshot(const neko::TabSnapshot &tab) {
  return TabPresentation{
      .id = static_cast<int>(tab.id),
      .title = QString::fromUtf8(tab.title),
      .path = QString::fromUtf8(tab.path),
      .pinned = tab.pinned,
      .modified = tab.modified,
  };
}

TabController::TabController(const TabControllerProps &props)
    : appState(props.appState) {}

neko::TabsSnapshot TabController::getTabsSnapshot() {
  return appState->get_tabs_snapshot();
}

QList<int> TabController::getCloseOtherTabIds(int tabId) const {
  auto rawIds = appState->get_close_other_tab_ids(tabId);

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

QList<int> TabController::getCloseAllTabIds() const {
  auto rawIds = appState->get_close_all_tab_ids();

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

QList<int> TabController::getCloseCleanTabIds() const {
  auto rawIds = appState->get_close_clean_tab_ids();

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

QList<int> TabController::getCloseLeftTabIds(int tabId) const {
  auto rawIds = appState->get_close_left_tab_ids(tabId);

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

QList<int> TabController::getCloseRightTabIds(int tabId) const {
  auto rawIds = appState->get_close_right_tab_ids(tabId);

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

int TabController::addTab() {
  if (appState == nullptr) {
    return -1;
  }

  auto result = appState->new_tab();
  int newTabId = static_cast<int>(result.id);
  int newTabIndex = static_cast<int>(result.index);

  auto presentation = fromSnapshot(result.snapshot);

  emit tabOpened(presentation, newTabIndex);
  emit activeTabChanged(newTabId);
  return newTabId;
}

bool TabController::closeTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  auto result = appState->close_tab(tabId);
  if (!result.closed) {
    return false;
  }

  emit tabClosed(tabId);

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  } else {
    emit allTabsClosed();
  }

  return true;
}

bool TabController::closeAllTabs() {
  auto result = appState->close_all_tabs();
  if (!result.success) {
    // TODO(scarlet): Show error?
    return false;
  }

  for (auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  } else {
    emit allTabsClosed();
  }

  return true;
}

bool TabController::closeCleanTabs() {
  auto result = appState->close_clean_tabs();
  if (!result.success) {
    // TODO(scarlet): Show error?
    return false;
  }

  for (auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  }

  return true;
}

bool TabController::closeOtherTabs(int tabId) {
  auto result = appState->close_other_tabs(tabId);
  if (!result.success) {
    // TODO(scarlet): Show error?
    return false;
  }

  for (auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  }

  return true;
}

bool TabController::closeLeftTabs(int tabId) {
  auto result = appState->close_left_tabs(tabId);
  if (!result.success) {
    // TODO(scarlet): Show error?
    return false;
  }

  for (auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  }

  return true;
}

bool TabController::closeRightTabs(int tabId) {
  auto result = appState->close_right_tabs(tabId);
  if (!result.success) {
    // TODO(scarlet): Show error?
    return false;
  }

  for (auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  }

  return true;
}

bool TabController::pinTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  auto result = appState->pin_tab(tabId);
  if (!result.success) {
    return false;
  }

  auto presentation = fromSnapshot(result.snapshot);

  emit tabUpdated(presentation);
  if (result.from_index != result.to_index) {
    emit tabMoved(static_cast<int>(result.from_index),
                  static_cast<int>(result.to_index));
  }

  return true;
}

bool TabController::unpinTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  auto result = appState->unpin_tab(tabId);
  if (!result.success) {
    return false;
  }

  auto presentation = fromSnapshot(result.snapshot);

  emit tabUpdated(presentation);
  if (result.from_index != result.to_index) {
    emit tabMoved(static_cast<int>(result.from_index),
                  static_cast<int>(result.to_index));
  }

  return true;
}

bool TabController::moveTab(int fromIndex, int toIndex) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->move_tab(fromIndex, toIndex)) {
    return false;
  }

  emit tabMoved(fromIndex, toIndex);
  return true;
}

void TabController::setActiveTab(int tabId) {
  if (appState == nullptr) {
    return;
  }

  appState->set_active_tab(tabId);
  emit activeTabChanged(tabId);
}

bool TabController::saveTabWithId(int tabId) const {
  return appState->save_tab_with_id(tabId);
}

bool TabController::saveTabWithIdAndSetPath(int tabId,
                                            const std::string &path) const {
  return appState->save_tab_with_id_and_set_path(tabId, path);
}

void TabController::setTabScrollOffsets(
    int tabId, const neko::ScrollOffsetFfi &newOffsets) {
  appState->set_tab_scroll_offsets(tabId, newOffsets);
}

#include "tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"

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

  int newTabId = static_cast<int>(appState->new_tab());
  emit tabListChanged();
  emit activeTabChanged(newTabId);
  return newTabId;
}

bool TabController::closeTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_tab(tabId)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(static_cast<int>(snapshot.active_id));

  return true;
}

bool TabController::closeAllTabs() {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_all_tabs()) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeCleanTabs() {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_clean_tabs()) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeOtherTabs(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_other_tabs(tabId)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeLeftTabs(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_left_tabs(tabId)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeRightTabs(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->close_right_tabs(tabId)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::pinTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->pin_tab(tabId)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(static_cast<int>(snapshot.active_id));
  return true;
}

bool TabController::unpinTab(int tabId) {
  if (appState == nullptr) {
    return false;
  }

  if (!appState->unpin_tab(tabId)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(static_cast<int>(snapshot.active_id));
  return true;
}

bool TabController::moveTab(int fromIndex, int toIndex) {
  if (appState == nullptr) {
    return false;
  }

  const auto beforeSnapshot = getTabsSnapshot();
  int count = static_cast<int>(beforeSnapshot.tabs.size());
  if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex > count) {
    return false;
  }

  if (!appState->move_tab(fromIndex, toIndex)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(static_cast<int>(snapshot.active_id));

  return true;
}

void TabController::setActiveTab(int tabId) {
  if (appState == nullptr) {
    return;
  }

  appState->set_active_tab(tabId);
  emit activeTabChanged(tabId);
}

bool TabController::saveActiveTab() const {
  return appState->save_active_tab();
}

bool TabController::saveActiveTabAndSetPath(const std::string &path) const {
  return appState->save_active_tab_and_set_path(path);
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

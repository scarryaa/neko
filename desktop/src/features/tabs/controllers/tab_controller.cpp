#include "tab_controller.h"

TabController::TabController(neko::AppState *appState) : appState(appState) {}

TabController::~TabController() {}

const neko::TabsSnapshot TabController::getTabsSnapshot() {
  return appState->get_tabs_snapshot();
}

const QList<int> TabController::getCloseOtherTabIds(int id) const {
  auto rawIds = appState->get_close_other_tab_ids(id);

  QList<int> ids = QList<int>();
  ids.reserve(rawIds.size());

  for (int i = 0; i < rawIds.size(); i++) {
    ids.append(rawIds[i]);
  }

  return ids;
}

const QList<int> TabController::getCloseLeftTabIds(int id) const {
  auto rawIds = appState->get_close_left_tab_ids(id);

  QList<int> ids = QList<int>();
  ids.reserve(rawIds.size());

  for (int i = 0; i < rawIds.size(); i++) {
    ids.append(rawIds[i]);
  }

  return ids;
}

const QList<int> TabController::getCloseRightTabIds(int id) const {
  auto rawIds = appState->get_close_right_tab_ids(id);

  QList<int> ids = QList<int>();
  ids.reserve(rawIds.size());

  for (int i = 0; i < rawIds.size(); i++) {
    ids.append(rawIds[i]);
  }

  return ids;
}

int TabController::addTab() {
  if (!appState) {
    return -1;
  }

  int id = appState->new_tab();
  emit tabListChanged();
  emit activeTabChanged(id);
  return id;
}

bool TabController::closeTab(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->close_tab(id)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(snapshot.active_id);

  return true;
}

bool TabController::closeOtherTabs(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->close_other_tabs(id)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeLeftTabs(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->close_left_tabs(id)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeRightTabs(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->close_right_tabs(id)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::pinTab(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->pin_tab(id)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(snapshot.active_id);
  return true;
}

bool TabController::unpinTab(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->unpin_tab(id)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(snapshot.active_id);
  return true;
}

bool TabController::moveTab(int from, int to) {
  if (!appState) {
    return false;
  }

  const auto beforeSnapshot = getTabsSnapshot();
  int count = beforeSnapshot.tabs.size();
  if (from < 0 || from >= count || to < 0 || to > count) {
    return false;
  }

  if (!appState->move_tab(from, to)) {
    return false;
  }

  const auto snapshot = getTabsSnapshot();

  emit tabListChanged();
  emit activeTabChanged(snapshot.active_id);

  return true;
}

void TabController::setActiveTab(int id) {
  if (!appState) {
    return;
  }

  appState->set_active_tab(id);
  emit activeTabChanged(id);
}

const bool TabController::saveActiveTab() const {
  return appState->save_active_tab();
}

const bool
TabController::saveActiveTabAndSetPath(const std::string &path) const {
  return appState->save_active_tab_and_set_path(path);
}

const bool TabController::saveTabWithId(int id) const {
  return appState->save_tab_with_id(id);
}

const bool
TabController::saveTabWithIdAndSetPath(int id, const std::string &path) const {
  return appState->save_tab_with_id_and_set_path(id, path);
}

void TabController::setTabScrollOffsets(
    int id, const neko::ScrollOffsetFfi &newOffsets) {
  appState->set_tab_scroll_offsets(id, newOffsets);
}

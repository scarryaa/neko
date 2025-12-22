#include "tab_controller.h"

TabController::TabController(neko::AppState *appState) : appState(appState) {}

TabController::~TabController() {}

const int TabController::getTabCount() const {
  return !appState ? 0 : appState->get_tab_count();
}

const int TabController::getActiveTabId() const {
  return !appState ? -1 : appState->get_active_tab_id();
}

const rust::Vec<rust::String> TabController::getTabTitles() const {
  if (!appState) {
    return {};
  }

  return appState->get_tab_titles();
}

const rust::Vec<bool> TabController::getTabModifiedStates() const {
  if (!appState) {
    return {};
  }

  return appState->get_tab_modified_states();
}

const rust::Vec<bool> TabController::getTabPinnedStates() const {
  if (!appState) {
    return {};
  }

  return appState->get_tab_pinned_states();
}

const bool TabController::getTabModified(int id) const {
  return appState->get_tab_modified(id);
}

const bool TabController::getTabWithPathExists(const std::string &path) const {
  return appState->tab_with_path_exists(path);
}

const int TabController::getTabIndexByPath(const std::string &path) const {
  return appState->get_tab_index_by_path(path);
}

const bool TabController::getTabsEmpty() const {
  return appState->get_tabs_empty();
}

const bool TabController::getIsPinned(int id) const {
  return appState->get_tab_pinned(id);
}

const int TabController::getTabId(int index) const {
  return appState->get_tab_id(index);
}

const QString TabController::getTabTitle(int id) const {
  const auto rawTitle = appState->get_tab_title(id);
  const QString title = QString::fromUtf8(rawTitle);

  return title;
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

  emit tabListChanged();

  int newActive = appState->get_active_tab_id();
  emit activeTabChanged(newActive);
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

  emit tabListChanged();
  emit activeTabChanged(appState->get_active_tab_id());
  return true;
}

bool TabController::unpinTab(int id) {
  if (!appState) {
    return false;
  }

  if (!appState->unpin_tab(id)) {
    return false;
  }

  emit tabListChanged();
  emit activeTabChanged(appState->get_active_tab_id());
  return true;
}

bool TabController::moveTab(int from, int to) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (from < 0 || from >= count || to < 0 || to > count) {
    return false;
  }

  if (!appState->move_tab(from, to)) {
    return false;
  }

  emit tabListChanged();
  emit activeTabChanged(appState->get_active_tab_id());

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

const QString TabController::getTabPath(int id) const {
  const auto rawPath = appState->get_tab_path(id);
  const QString path = QString::fromUtf8(rawPath);

  return path;
}

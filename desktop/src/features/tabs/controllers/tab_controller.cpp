#include "tab_controller.h"

TabController::TabController(neko::AppState *appState) : appState(appState) {}

TabController::~TabController() {}

int TabController::addTab() {
  if (!appState) {
    return -1;
  }

  int index = appState->new_tab();
  emit tabListChanged();
  emit activeTabChanged(index);
  return index;
}

bool TabController::closeTab(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count) {
    return false;
  }

  if (!appState->close_tab(index)) {
    return false;
  }

  emit tabListChanged();

  int newActive = appState->get_active_tab_index();
  emit activeTabChanged(newActive);
  return true;
}

bool TabController::closeOtherTabs(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count) {
    return false;
  }

  if (!appState->close_other_tabs(index)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeLeftTabs(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count || index == 0) {
    return false;
  }

  if (!appState->close_left_tabs(index)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::closeRightTabs(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count || index == count - 1) {
    return false;
  }

  if (!appState->close_right_tabs(index)) {
    return false;
  }

  emit tabListChanged();

  return true;
}

bool TabController::pinTab(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count) {
    return false;
  }

  if (!appState->pin_tab(index)) {
    return false;
  }

  emit tabListChanged();
  emit activeTabChanged(appState->get_active_tab_index());
  return true;
}

bool TabController::unpinTab(int index) {
  if (!appState) {
    return false;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count) {
    return false;
  }

  if (!appState->unpin_tab(index)) {
    return false;
  }

  emit tabListChanged();
  emit activeTabChanged(appState->get_active_tab_index());
  return true;
}

const int TabController::getTabCount() const {
  return !appState ? 0 : appState->get_tab_count();
}

const int TabController::getActiveTabIndex() const {
  return !appState ? -1 : appState->get_active_tab_index();
}

void TabController::setActiveTabIndex(int index) {
  if (!appState) {
    return;
  }

  int count = appState->get_tab_count();
  if (index < 0 || index >= count) {
    return;
  }

  appState->set_active_tab_index(index);
  emit activeTabChanged(index);
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

const bool TabController::getTabModified(int index) const {
  return appState->get_tab_modified(index);
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

const QString TabController::getTabPath(int index) const {
  const auto rawPath = appState->get_tab_path(index);
  const QString path = QString::fromUtf8(rawPath);

  return path;
}

const bool TabController::saveActiveTab() const {
  return appState->save_active_tab();
}

const bool
TabController::saveActiveTabAndSetPath(const std::string &path) const {
  return appState->save_active_tab_and_set_path(path);
}

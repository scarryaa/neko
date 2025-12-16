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

int TabController::getTabCount() const {
  return !appState ? 0 : appState->get_tab_count();
}

int TabController::getActiveTabIndex() const {
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

rust::Vec<rust::String> TabController::getTabTitles() const {
  if (!appState) {
    return {};
  }
  return appState->get_tab_titles();
}

rust::Vec<bool> TabController::getTabModifiedStates() const {
  if (!appState) {
    return {};
  }
  return appState->get_tab_modified_states();
}

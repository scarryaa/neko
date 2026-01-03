#include "tab_bridge.h"
#include "neko-core/src/ffi/bridge.rs.h"

TabBridge::TabBridge(TabBridgeProps props)
    : tabController(std::move(props.tabController)) {}

TabPresentation TabBridge::fromSnapshot(const neko::TabSnapshot &tab) {
  return TabPresentation{
      .id = static_cast<int>(tab.id),
      .title = QString::fromUtf8(tab.title),
      .path = QString::fromUtf8(tab.path),
      .pinned = tab.pinned,
      .modified = tab.modified,
      .scrollOffsets =
          TabScrollOffsets{.x = static_cast<double>(tab.scroll_offsets.x),
                           .y = static_cast<double>(tab.scroll_offsets.y)},
  };
}

neko::TabsSnapshot TabBridge::getTabsSnapshot() {
  return tabController->get_tabs_snapshot();
}

neko::TabSnapshotMaybe TabBridge::getTabSnapshot(int tabId) {
  return tabController->get_tab_snapshot(tabId);
}

QList<int>
TabBridge::getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                          int anchorTabId, bool closePinned) const {
  const auto result =
      tabController->get_close_tab_ids(operationType, anchorTabId, closePinned);
  return {result.begin(), result.end()};
}

int TabBridge::createDocumentTabAndView(const std::string &title,
                                        bool addTabToHistory,
                                        bool activateView) {
  auto result = tabController->create_document_tab_and_view(
      title, addTabToHistory, activateView);

  const int newTabId = static_cast<int>(result.tab_id);
  auto tabsSnapshot = tabController->get_tabs_snapshot();

  int newTabIndex = -1;
  std::optional<TabPresentation> presentation;

  for (int i = 0; i < tabsSnapshot.tabs.size(); ++i) {
    const auto &tabSnapshot = tabsSnapshot.tabs[i];

    if (static_cast<int>(tabSnapshot.id) == newTabId) {
      newTabIndex = i;
      presentation = fromSnapshot(tabSnapshot);
      break;
    }
  }

  if (newTabIndex < 0 || !presentation.has_value()) {
    return -1;
  }

  emit tabOpened(presentation.value(), newTabIndex);
  emit activeTabChanged(newTabId);

  return newTabId;
}

// TODO(scarlet): Figure out a unified/better solution than separate 'core ->
// cpp' signals?
void TabBridge::notifyTabOpenedFromCore(int tabId) {
  auto tabsSnapshot = getTabsSnapshot();
  const neko::TabSnapshot *found = nullptr;
  int index = 0;

  for (int i = 0; i < static_cast<int>(tabsSnapshot.tabs.size()); ++i) {
    const auto &tab = tabsSnapshot.tabs[i];
    if (static_cast<int>(tab.id) == tabId) {
      found = &tab;
      index = i;
      break;
    }
  }

  if (found == nullptr) {
    return;
  }

  TabPresentation presentation = fromSnapshot(*found);

  emit tabOpened(presentation, index);
  emit activeTabChanged(tabId);
}

void TabBridge::fileOpened(const neko::TabSnapshot &snapshot) {
  auto presentation = fromSnapshot(snapshot);

  emit tabOpened(presentation, presentation.id);
  emit activeTabChanged(presentation.id);
}

void TabBridge::tabSaved(int tabId) {
  const auto snapshotMaybe = tabController->get_tab_snapshot(tabId);

  if (snapshotMaybe.found) {
    auto presentation = fromSnapshot(snapshotMaybe.snapshot);
    emit tabUpdated(presentation);
  }
}

bool TabBridge::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                          int anchorTabId, bool closePinned) {
  auto result =
      tabController->close_tabs(operationType, anchorTabId, closePinned);
  if (result.closed_ids.empty()) {
    return false;
  }

  for (const auto closedTabId : result.closed_ids) {
    emit tabClosed(static_cast<int>(closedTabId));
  }

  if (result.has_active) {
    emit activeTabChanged(static_cast<int>(result.active_id));
  } else {
    emit allTabsClosed();
  }

  return true;
}

bool TabBridge::pinTab(int tabId) {
  auto result = tabController->pin_tab(tabId);
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

bool TabBridge::unpinTab(int tabId) {
  auto result = tabController->unpin_tab(tabId);
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

bool TabBridge::moveTabBy(neko::Buffer buffer, int delta, bool useHistory) {
  // if (tabCoreApi == nullptr) {
  //   return false;
  // }

  // neko::MoveActiveTabResult result =
  //     tabCoreApi->moveTabBy(buffer, delta, useHistory);
  // int tabId = static_cast<int>(result.id);
  // TabScrollOffsets offsets;
  //
  // if (result.reopened) {
  //   TabPresentation presentation = fromSnapshot(result.snapshot);
  //   auto snapshot = getTabsSnapshot();
  //   offsets = presentation.scrollOffsets;
  //
  //   // Get the tab index
  //   int index = 0;
  //   for (int i = 0; i < static_cast<int>(snapshot.tabs.size()); ++i) {
  //     if (static_cast<int>(snapshot.tabs[i].id) == tabId) {
  //       index = i;
  //       break;
  //     }
  //   }
  //
  //   emit tabOpened(presentation, index);
  // }
  //
  // emit activeTabChanged(tabId);
  //
  // // Restore offsets after emitting activeTabChanged, since the handler tries
  // to
  // // restore scroll offsets on active change
  // if (result.reopened) {
  //   emit restoreScrollOffsetsForReopenedTab(offsets);
  // }

  return true;
}

bool TabBridge::moveTab(int fromIndex, int toIndex) {
  if (!tabController->move_tab(fromIndex, toIndex)) {
    return false;
  }

  emit tabMoved(fromIndex, toIndex);
  return true;
}

void TabBridge::setActiveTab(int tabId) {
  tabController->set_active_tab(tabId);

  emit activeTabChanged(tabId);
}

void TabBridge::setTabScrollOffsets(int tabId,
                                    const neko::ScrollOffsetFfi &newOffsets) {
  tabController->set_tab_scroll_offsets(tabId, newOffsets);
}

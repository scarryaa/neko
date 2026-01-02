#include "tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"

TabPresentation TabController::fromSnapshot(const neko::TabSnapshot &tab) {
  return TabPresentation{
      .id = static_cast<int>(tab.id),
      .pinned = tab.pinned,
      .scrollOffsets =
          TabScrollOffsets{.x = static_cast<double>(tab.scroll_offsets.x),
                           .y = static_cast<double>(tab.scroll_offsets.y)},
  };
}

TabController::TabController(const TabControllerProps &props)
    : tabCoreApi(props.tabCoreApi) {}

neko::TabsSnapshot TabController::getTabsSnapshot() {
  return tabCoreApi->getTabsSnapshot();
}

QList<int>
TabController::getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                              int anchorTabId, bool closePinned) const {
  auto rawIds =
      tabCoreApi->getCloseTabIds(operationType, anchorTabId, closePinned);

  QList<int> ids = QList<int>();
  ids.reserve(static_cast<int>(rawIds.size()));

  for (uint64_t rawId : rawIds) {
    ids.append(static_cast<int>(rawId));
  }

  return ids;
}

int TabController::addTab() {
  if (tabCoreApi == nullptr) {
    return -1;
  }

  auto result = tabCoreApi->createDocumentTabAndView("Untitled", true, true);
  const int newTabId = static_cast<int>(result.tab_id);
  auto tabsSnapshot = tabCoreApi->getTabsSnapshot();

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
void TabController::notifyTabOpenedFromCore(int tabId) {
  if (tabCoreApi == nullptr) {
    return;
  }

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

void TabController::fileOpened(const neko::TabSnapshot &snapshot) {
  auto presentation = fromSnapshot(snapshot);
  emit tabUpdated(presentation);
}

void TabController::tabSaved(int tabId) {
  const auto snapshotMaybe = tabCoreApi->getTabSnapshot(tabId);

  if (snapshotMaybe.found) {
    auto presentation = fromSnapshot(snapshotMaybe.snapshot);
    emit tabUpdated(presentation);
  }
}

bool TabController::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                              int anchorTabId, bool closePinned) {
  if (tabCoreApi == nullptr) {
    // TODO(scarlet): Show error?
    return false;
  }

  auto result = tabCoreApi->closeTabs(operationType, anchorTabId, closePinned);
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

bool TabController::pinTab(int tabId) {
  if (tabCoreApi == nullptr) {
    return false;
  }

  auto result = tabCoreApi->pinTab(tabId);
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
  if (tabCoreApi == nullptr) {
    return false;
  }

  auto result = tabCoreApi->unpinTab(tabId);
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

bool TabController::moveTabBy(int delta, bool useHistory) {
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

bool TabController::moveTab(int fromIndex, int toIndex) {
  if (tabCoreApi == nullptr) {
    return false;
  }

  if (!tabCoreApi->moveTab(fromIndex, toIndex)) {
    return false;
  }

  emit tabMoved(fromIndex, toIndex);
  return true;
}

void TabController::setActiveTab(int tabId) {
  if (tabCoreApi == nullptr) {
    return;
  }

  tabCoreApi->setActiveTab(tabId);

  emit activeTabChanged(tabId);
}

void TabController::setTabScrollOffsets(
    int tabId, const neko::ScrollOffsetFfi &newOffsets) {
  tabCoreApi->setTabScrollOffsets(tabId, newOffsets);
}

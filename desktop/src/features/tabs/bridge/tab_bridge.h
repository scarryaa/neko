#ifndef TAB_BRIDGE_H
#define TAB_BRIDGE_H

#include "features/tabs/types/types.h"
#include <QList>
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class TabBridge : public QObject {
  Q_OBJECT

public:
  struct TabBridgeProps {
    rust::Box<neko::TabController> tabController;
  };

  explicit TabBridge(TabBridgeProps props);
  ~TabBridge() override = default;

  // Getters
  [[nodiscard]] QList<int>
  getCloseTabIds(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool closePinned) const;
  neko::TabsSnapshot getTabsSnapshot();
  neko::TabSnapshotMaybe getTabSnapshot(int tabId);

  // Setters
  int createDocumentTabAndView(const std::string &title, bool addTabToHistory,
                               bool activateView);
  bool closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool closePinned);
  bool pinTab(int tabId);
  bool unpinTab(int tabId);
  bool moveTab(int fromIndex, int toIndex);
  static bool moveTabBy(neko::Buffer buffer, int delta, bool useHistory);
  void setActiveTab(int tabId);
  void setTabScrollOffsets(int tabId, const neko::ScrollOffsetFfi &newOffsets);
  void notifyTabOpenedFromCore(int tabId);

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void fileOpened(const neko::TabSnapshot &snapshot);
  void tabSaved(int tabId);

signals:
  void tabOpened(const TabPresentation &tab, int index);
  void tabClosed(int tabId);
  void tabMoved(int fromIndex, int toIndex);
  void
  tabUpdated(const TabPresentation &tab); // updated title/path/pinned/modified
  void restoreScrollOffsetsForReopenedTab(const TabScrollOffsets &offsets);
  void activeTabChanged(int tabId);
  void allTabsClosed();

private:
  static TabPresentation fromSnapshot(const neko::TabSnapshot &tab);

  rust::Box<neko::TabController> tabController;
};

#endif

#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include "core/api/tab_core_api.h"
#include "features/tabs/types/types.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <QObject>

class TabController : public QObject {
  Q_OBJECT

public:
  struct TabControllerProps {
    ITabCoreApi *tabCoreApi;
  };

  explicit TabController(const TabControllerProps &props);
  ~TabController() override = default;

  // Getters
  [[nodiscard]] QList<int>
  getCloseTabIds(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool closePinned) const;
  neko::TabsSnapshot getTabsSnapshot();

  // Setters
  int addTab();
  bool closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool closePinned);
  bool pinTab(int tabId);
  bool unpinTab(int tabId);
  bool moveTabBy(int delta, bool useHistory);
  bool moveTab(int fromIndex, int toIndex);
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

  ITabCoreApi *tabCoreApi;
};

#endif

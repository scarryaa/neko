#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include "features/tabs/tab_types.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <QObject>
#include <string>

class TabController : public QObject {
  Q_OBJECT

public:
  struct TabControllerProps {
    neko::AppState *appState;
  };

  explicit TabController(const TabControllerProps &props);
  ~TabController() override = default;

  // Getters
  [[nodiscard]] QList<int> getCloseOtherTabIds(int tabId) const;
  [[nodiscard]] QList<int> getCloseLeftTabIds(int tabId) const;
  [[nodiscard]] QList<int> getCloseRightTabIds(int tabId) const;
  [[nodiscard]] QList<int> getCloseAllTabIds() const;
  [[nodiscard]] QList<int> getCloseCleanTabIds() const;
  neko::TabsSnapshot getTabsSnapshot();

  // Setters
  int addTab();
  bool closeTab(int tabId);
  bool closeAllTabs();
  bool closeCleanTabs();
  bool closeOtherTabs(int tabId);
  bool closeLeftTabs(int tabId);
  bool closeRightTabs(int tabId);
  bool pinTab(int tabId);
  bool unpinTab(int tabId);
  bool moveTabBy(int delta);
  bool moveTab(int fromIndex, int toIndex);
  void setActiveTab(int tabId);
  [[nodiscard]] bool saveTabWithId(int tabId) const;
  [[nodiscard]] bool saveTabWithIdAndSetPath(int tabId,
                                             const std::string &path) const;
  void setTabScrollOffsets(int tabId, const neko::ScrollOffsetFfi &newOffsets);

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void fileOpened(const neko::TabSnapshot &snapshot);

signals:
  void tabOpened(const TabPresentation &tab, int index);
  void tabClosed(int tabId);
  void tabMoved(int fromIndex, int toIndex);
  void
  tabUpdated(const TabPresentation &tab); // updated title/path/pinned/modified
  void activeTabChanged(int tabId);
  void allTabsClosed();

private:
  static TabPresentation fromSnapshot(const neko::TabSnapshot &tab);

  neko::AppState *appState;
};

#endif

#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

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
  neko::TabsSnapshot getTabsSnapshot();
  [[nodiscard]] QList<int> getCloseOtherTabIds(int tabId) const;
  [[nodiscard]] QList<int> getCloseLeftTabIds(int tabId) const;
  [[nodiscard]] QList<int> getCloseRightTabIds(int tabId) const;

  // Setters
  int addTab();
  bool closeTab(int tabId);
  bool closeOtherTabs(int tabId);
  bool closeLeftTabs(int tabId);
  bool closeRightTabs(int tabId);
  bool pinTab(int tabId);
  bool unpinTab(int tabId);
  bool moveTab(int fromIndex, int toIndex);
  void setActiveTab(int tabId);
  [[nodiscard]] bool saveActiveTab() const;
  [[nodiscard]] bool saveActiveTabAndSetPath(const std::string &path) const;
  [[nodiscard]] bool saveTabWithId(int tabId) const;
  [[nodiscard]] bool saveTabWithIdAndSetPath(int tabId,
                                             const std::string &path) const;
  void setTabScrollOffsets(int tabId, const neko::ScrollOffsetFfi &newOffsets);

signals:
  void tabListChanged();
  void activeTabChanged(int tabId);

private:
  neko::AppState *appState;
};

#endif

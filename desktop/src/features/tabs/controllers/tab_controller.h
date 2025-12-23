#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class TabController : public QObject {
  Q_OBJECT

public:
  explicit TabController(neko::AppState *appState);
  ~TabController();

  // Getters
  const neko::TabsSnapshot getTabsSnapshot();
  const QList<int> getCloseOtherTabIds(int id) const;
  const QList<int> getCloseLeftTabIds(int id) const;
  const QList<int> getCloseRightTabIds(int id) const;

  // Setters
  int addTab();
  bool closeTab(int id);
  bool closeOtherTabs(int id);
  bool closeLeftTabs(int id);
  bool closeRightTabs(int id);
  bool pinTab(int id);
  bool unpinTab(int id);
  bool moveTab(int from, int to);
  void setActiveTab(int id);
  const bool saveActiveTab() const;
  const bool saveActiveTabAndSetPath(const std::string &path) const;
  const bool saveTabWithId(int id) const;
  const bool saveTabWithIdAndSetPath(int id, const std::string &path) const;
  void setTabScrollOffsets(int id, const neko::ScrollOffsetFfi &newOffsets);

signals:
  void tabListChanged();
  void activeTabChanged(int id);

private:
  neko::AppState *appState;
};

#endif

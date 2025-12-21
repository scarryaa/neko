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
  const int getTabCount() const;
  const int getActiveTabIndex() const;
  const rust::Vec<rust::String> getTabTitles() const;
  const rust::Vec<bool> getTabModifiedStates() const;
  const rust::Vec<bool> getTabPinnedStates() const;
  const bool getTabModified(int index) const;
  const bool getTabWithPathExists(const std::string &path) const;
  const int getTabIndexByPath(const std::string &path) const;
  const bool getTabsEmpty() const;
  const bool getIsPinned(int index) const;

  // Setters
  int addTab();
  bool closeTab(int index);
  bool closeOtherTabs(int index);
  bool closeLeftTabs(int index);
  bool closeRightTabs(int index);
  bool pinTab(int index);
  bool unpinTab(int index);
  bool moveTab(int from, int to);
  void setActiveTabIndex(int index);
  const bool saveActiveTab() const;
  const bool saveActiveTabAndSetPath(const std::string &path) const;
  const QString getTabPath(int index) const;

signals:
  void tabListChanged();
  void activeTabChanged(int index);

private:
  neko::AppState *appState;
};

#endif

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
  const int getActiveTabId() const;
  const rust::Vec<rust::String> getTabTitles() const;
  const rust::Vec<bool> getTabModifiedStates() const;
  const rust::Vec<bool> getTabPinnedStates() const;
  const bool getTabModified(int id) const;
  const bool getTabWithPathExists(const std::string &path) const;
  const int getTabIndexByPath(const std::string &path) const;
  const bool getTabsEmpty() const;
  const bool getIsPinned(int id) const;
  const int getTabId(int index) const;
  const QString getTabTitle(int id) const;

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
  const QString getTabPath(int id) const;

signals:
  void tabListChanged();
  void activeTabChanged(int id);

private:
  neko::AppState *appState;
};

#endif

#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/mod.rs.h>

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
  const bool getTabModified(int index) const;
  const bool getTabWithPathExists(const std::string &path) const;
  const int getTabIndexByPath(const std::string &path) const;
  const bool getTabsEmpty() const;

  // Setters
  int addTab();
  bool closeTab(int index);
  void setActiveTabIndex(int index);
  const bool saveActiveTab() const;
  const bool saveActiveTabAndSetPath(const std::string &path) const;

signals:
  void tabListChanged();
  void activeTabChanged(int index);

private:
  neko::AppState *appState;
};

#endif

#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/mod.rs.h>

class TabController : public QObject {
  Q_OBJECT

public:
  explicit TabController(neko::AppState *appState);
  ~TabController();

  int addTab();
  bool closeTab(int index);
  int getTabCount() const;
  int getActiveTabIndex() const;
  void setActiveTabIndex(int index);
  rust::Vec<rust::String> getTabTitles() const;
  rust::Vec<bool> getTabModifiedStates() const;

signals:
  void tabListChanged();
  void activeTabChanged(int index);

private:
  neko::AppState *appState;
};

#endif

#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/tabs/tab_widget.h"
#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStringList>

class TabBarWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit TabBarWidget(neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~TabBarWidget();

  void setTabs(QStringList titles, rust::Vec<bool> modifiedStates);
  void setCurrentIndex(size_t index);
  void setTabModified(int index, bool modified);
  int getNumberOfTabs();

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index, int numberOfTabs,
                         bool bypassConfirmation = false);
  void newTabRequested();

private:
  void updateTabAppearance();
  void updateViewport();

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QPushButton *newTabButton;
  QWidget *containerWidget;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  size_t currentTabIndex;
};

#endif // TAB_BAR_WIDGET_H

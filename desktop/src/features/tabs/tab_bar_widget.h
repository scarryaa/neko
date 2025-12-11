#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/tabs/tab_widget.h"
#include "utils/gui_utils.h"
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStringList>
#include <neko_core.h>

class TabBarWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit TabBarWidget(NekoConfigManager *manager,
                        NekoThemeManager *themeManager,
                        QWidget *parent = nullptr);
  ~TabBarWidget();

  void setTabs(QStringList titles, bool *modifiedStates);
  void setCurrentIndex(size_t index);
  void setTabModified(int index, bool modified);

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index);
  void newTabRequested();

private:
  void updateTabAppearance();
  void updateViewport();

  NekoConfigManager *configManager;
  NekoThemeManager *themeManager;
  QPushButton *newTabButton;
  QWidget *containerWidget;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  size_t currentTabIndex;
};

#endif // TAB_BAR_WIDGET_H

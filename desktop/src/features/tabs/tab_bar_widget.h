#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/tabs/tab_widget.h"
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
  static constexpr int HEIGHT = 32;
  static constexpr QColor COLOR_BLACK = QColor(0, 0, 0);
  QColor BORDER_COLOR = QColor("#3c3c3c");

  explicit TabBarWidget(QWidget *parent = nullptr);
  ~TabBarWidget();

  void setTabs(QStringList titles);
  void setCurrentIndex(size_t index);

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index);
  void newTabRequested();

private:
  void updateTabAppearance();
  void updateViewport();

  QPushButton *newTabButton;
  QWidget *containerWidget;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  size_t currentTabIndex;
};

#endif // TAB_BAR_WIDGET_H

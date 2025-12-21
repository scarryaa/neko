#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "utils/gui_utils.h"
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStringList>

class TabBarWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit TabBarWidget(neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        ContextMenuRegistry &contextMenuRegistry,
                        CommandRegistry &commandRegistry,
                        TabController *tabController,
                        QWidget *parent = nullptr);
  ~TabBarWidget();

  void applyTheme();
  void setTabs(QStringList titles, QStringList paths,
               rust::Vec<bool> modifiedStates, rust::Vec<bool> pinnedStates);
  void setCurrentIndex(size_t index);
  void setTabModified(int index, bool modified);
  int getNumberOfTabs();

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index, int numberOfTabs,
                         bool bypassConfirmation = false);
  void newTabRequested();
  void tabPinnedChanged(int index);
  void tabUnpinRequested(int index);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  int dropIndexForPosition(const QPoint &pos) const;
  void updateDropIndicator(int index);
  void updateTabAppearance();
  void registerCommands();

  TabController *tabController;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QPushButton *newTabButton;
  QWidget *containerWidget;
  QWidget *dropIndicator;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  size_t currentTabIndex;
};

#endif // TAB_BAR_WIDGET_H

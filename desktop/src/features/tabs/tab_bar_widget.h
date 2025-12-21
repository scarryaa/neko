#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_widget.h"
#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QClipboard>
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
                        ContextMenuRegistry &contextMenuRegistry,
                        CommandRegistry &commandRegistry,
                        TabController *tabController,
                        QWidget *parent = nullptr);
  ~TabBarWidget();

  void applyTheme();
  void setTabs(QStringList titles, QStringList paths,
               rust::Vec<bool> modifiedStates);
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
  void registerCommands();

  TabController *tabController;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QPushButton *newTabButton;
  QWidget *containerWidget;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  size_t currentTabIndex;
};

#endif // TAB_BAR_WIDGET_H

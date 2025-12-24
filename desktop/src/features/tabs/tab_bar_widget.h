#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "theme/theme_types.h"
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
                        const TabBarTheme &tabBarTheme,
                        const TabTheme &tabTheme,
                        ContextMenuRegistry &contextMenuRegistry,
                        CommandRegistry &commandRegistry,
                        TabController *tabController,
                        QWidget *parent = nullptr);
  ~TabBarWidget() override = default;

  void setAndApplyTheme(const TabBarTheme &newTheme);
  void setAndApplyTabTheme(const TabTheme &newTheme);
  void setTabs(const QStringList &titles, const QStringList &paths,
               const QList<bool> &modifiedStates,
               const QList<bool> &pinnedStates);
  void setCurrentId(int tabId);
  void setTabModified(int tabId, bool modified);
  int getNumberOfTabs();

signals:
  void currentChanged(int tabId);
  void tabCloseRequested(int tabId, bool bypassConfirmation = false);
  void newTabRequested();
  void tabPinnedChanged(int tabId);
  void tabUnpinRequested(int tabId);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  [[nodiscard]] int dropIndexForPosition(const QPoint &pos) const;
  void updateDropIndicator(int index);
  void updateTabAppearance();
  void registerCommands();

  TabController *tabController;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  neko::ConfigManager &configManager;
  QPushButton *newTabButton;
  QWidget *containerWidget;
  QWidget *dropIndicator;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  int currentTabId;

  TabBarTheme tabBarTheme;
  TabTheme tabTheme;

  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
};

#endif // TAB_BAR_WIDGET_H

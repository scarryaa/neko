#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

class CommandRegistry;
class ContextMenuRegistry;
class TabController;
class TabWidget;
class ThemeProvider;

#include "theme/theme_types.h"
#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QList>
#include <QScrollArea>
#include <QStringList>

QT_FWD(QPushButton, QHBoxLayout, QDragEnterEvent, QDragMoveEvent,
       QDragLeaveEvent, QDropEvent, QEvent, QPoint);

class TabBarWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit TabBarWidget(neko::ConfigManager &configManager,
                        const TabBarTheme &tabBarTheme,
                        const TabTheme &tabTheme, ThemeProvider *themeProvider,
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

  ThemeProvider *themeProvider;
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

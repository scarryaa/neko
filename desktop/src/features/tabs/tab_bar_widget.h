#ifndef TAB_BAR_WIDGET_H
#define TAB_BAR_WIDGET_H

class CommandRegistry;
class ContextMenuRegistry;
class TabController;
class TabWidget;
class ThemeProvider;

#include "features/tabs/tab_types.h"
#include "theme/theme_types.h"
#include "types/qt_types_fwd.h"
#include <QScrollArea>
#include <QStringList>

QT_FWD(QPushButton, QHBoxLayout, QDragEnterEvent, QDragMoveEvent,
       QDragLeaveEvent, QDropEvent, QEvent, QPoint);

class TabBarWidget : public QScrollArea {
  Q_OBJECT

public:
  struct TabBarProps {
    TabBarTheme theme;
    TabTheme tabTheme;
    QFont font;
    ThemeProvider *themeProvider;
    ContextMenuRegistry *contextMenuRegistry;
    CommandRegistry *commandRegistry;
    TabController *tabController;
  };

  explicit TabBarWidget(const TabBarProps &props, QWidget *parent = nullptr);
  ~TabBarWidget() override = default;

  void setAndApplyTheme(const TabBarTheme &newTheme);
  void setAndApplyTabTheme(const TabTheme &newTheme);
  void setTabModified(int tabId, bool modified);
  int getNumberOfTabs();

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void addTab(const TabPresentation &tab, int index);
  void removeTab(int tabId);
  void moveTab(int fromIndex, int toIndex);
  void
  updateTab(const TabPresentation &tab); // updates title/path/pinned/modified
  void setCurrentTabId(int tabId);

signals:
  void currentChanged(int tabId);
  void tabCloseRequested(int tabId, bool bypassConfirmation = false);
  void newTabRequested();
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
  TabWidget *findTabWidgetById(int tabId);

  ThemeProvider *themeProvider;
  TabController *tabController;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  QWidget *containerWidget;
  QWidget *dropIndicator;
  QHBoxLayout *layout;
  QList<TabWidget *> tabs;
  int currentTabId;

  QFont font;
  TabBarTheme tabBarTheme;
  TabTheme tabTheme;

  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
};

#endif // TAB_BAR_WIDGET_H

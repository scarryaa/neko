#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/context_menu/providers/tab_context.h"
#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QDrag>
#include <QIcon>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QWidget>

class TabWidget : public QWidget {
  Q_OBJECT

public:
  using GetTabCountFn = std::function<int()>;

  explicit TabWidget(const QString &title, const QString &path, int index,
                     bool isPinned, neko::ConfigManager &configManager,
                     neko::ThemeManager &themeManager,
                     ContextMenuRegistry &contextMenuRegistry,
                     CommandRegistry &commandRegistry,
                     GetTabCountFn getTabCount, QWidget *parent = nullptr);
  ~TabWidget();

  void setActive(bool active);
  void setModified(bool modified);
  void setIsPinned(bool isPinned);
  bool getIsPinned() const;

signals:
  void clicked();
  void closeRequested(bool bypassConfirmation);
  void unpinRequested();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  double measureText(QString text);

  GetTabCountFn getTabCount_;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  ContextMenuWidget *contextMenuWidget;
  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QString title;
  QString path;
  bool isModified = false;
  int index;
  bool isPinned = false;
  bool isActive;
  bool isHovered = false;
  bool isCloseHovered = false;
  bool dragEligible = false;
  bool dragInProgress = false;
  QPoint dragStartPosition;
};

#endif // TAB_WIDGET_H

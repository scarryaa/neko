#ifndef CONTEXT_MENU_WIDGET_H
#define CONTEXT_MENU_WIDGET_H

#include "features/context_menu/context_menu_item.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <neko-core/src/ffi/mod.rs.h>

class ContextMenuWidget : public QWidget {
  Q_OBJECT

public:
  explicit ContextMenuWidget(neko::ThemeManager *themeManager,
                             neko::ConfigManager *configManager,
                             QWidget *parent = nullptr);
  ~ContextMenuWidget() override;

  void setItems(const QVector<ContextMenuItem> &items);
  void showMenu(const QPoint &position);

signals:
  void actionTriggered(const QString &id);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void clearRows();

  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  QVBoxLayout *layout;
};

#endif

#ifndef CONTEXT_MENU_WIDGET_H
#define CONTEXT_MENU_WIDGET_H

#include "features/context_menu/context_menu_frame.h"
#include "features/context_menu/context_menu_item.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
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
  void keyPressEvent(QKeyEvent *event) override;

private:
  void clearRows();

  ContextMenuFrame *mainFrame;
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  QVBoxLayout *layout;

  static constexpr double SHADOW_X_OFFSET = 0.0;
  static constexpr double SHADOW_Y_OFFSET = 5.0;
  static constexpr double SHADOW_BLUR_RADIUS = 25.0;
  static constexpr double CONTENT_MARGIN =
      20.0; // Content margin for drop shadow
};

#endif

#include "context_menu_frame.h"
#include "utils/gui_utils.h"

ContextMenuFrame::ContextMenuFrame(neko::ThemeManager &themeManager,
                                   QWidget *parent)
    : QFrame(parent), themeManager(themeManager) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

void ContextMenuFrame::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill =
      UiUtils::getThemeColor(themeManager, "context_menu.background");
  const QColor stroke =
      UiUtils::getThemeColor(themeManager, "context_menu.border");

  QPainterPath path;
  const constexpr qreal radius = 12.0;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
  painter.fillPath(path, fill);
  const QPen pen(stroke, 1);
  painter.setPen(pen);
  painter.drawPath(path);
}

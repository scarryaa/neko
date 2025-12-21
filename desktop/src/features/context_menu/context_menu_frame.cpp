#include "context_menu_frame.h"

ContextMenuFrame::ContextMenuFrame(neko::ThemeManager &themeManager,
                                   QWidget *parent)
    : QFrame(parent), themeManager(themeManager) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

ContextMenuFrame::~ContextMenuFrame() {}

void ContextMenuFrame::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill = UiUtils::getThemeColor(themeManager, "ui.background");
  const QColor stroke = UiUtils::getThemeColor(themeManager, "ui.border");

  QPainterPath path;
  const constexpr qreal r = 12.0;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), r, r);
  p.fillPath(path, fill);
  const QPen pen(stroke, 1);
  p.setPen(pen);
  p.drawPath(path);
}

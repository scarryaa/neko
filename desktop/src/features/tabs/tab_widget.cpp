#include "tab_widget.h"
#include "utils/gui_utils.h"

TabWidget::TabWidget(const QString &title, int index,
                     NekoConfigManager *configManager,
                     NekoThemeManager *themeManager, QWidget *parent)
    : QWidget(parent), configManager(configManager), themeManager(themeManager),
      title(title), index(index), isActive(false) {
  QFont uiFont = UiUtils::loadFont(configManager, UiUtils::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;
  setFixedHeight(dynamicHeight);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setMouseTracking(true);

  double titleWidth = measureText(title);
  setMinimumWidth(12 + titleWidth + 44);
}

void TabWidget::setActive(bool active) {
  isActive = active;
  update();
}

void TabWidget::setModified(bool modified) {
  isModified = modified;
  update();
}

double TabWidget::measureText(QString text) {
  return fontMetrics().horizontalAdvance(text);
}

void TabWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setFont(font());

  QString borderColor = UiUtils::getThemeColor(themeManager, "ui.border");
  QString bgColor = UiUtils::getThemeColor(themeManager, "tab.inactive");
  QString hoverColor = UiUtils::getThemeColor(themeManager, "tab.hover");
  QString activeColor = UiUtils::getThemeColor(themeManager, "tab.active");
  QString foregroundColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground");
  QString foregroundMutedColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground_muted");
  QString modifiedColor = UiUtils::getThemeColor(themeManager, "ui.accent");

  // Background
  if (isActive) {
    painter.setBrush(activeColor);
  } else if (isHovered) {
    painter.setBrush(hoverColor);
  } else {
    painter.setBrush(bgColor);
  }
  painter.setPen(Qt::NoPen);
  painter.drawRect(rect());

  // Right border
  painter.setPen(QPen(borderColor));
  painter.drawLine(width() - 1, 0, width() - 1, height());

  // Bottom border
  if (!isActive) {
    painter.setPen(QPen(borderColor));
    painter.drawLine(0, height() - 1, width(), height() - 1);
  }

  // Text
  QRect textRect = rect().adjusted(12, 0, -30, 0);
  painter.setPen(isActive ? foregroundColor : foregroundMutedColor);
  painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

  // Modified marker
  if (isModified) {
    QRect modifiedRect(width() - 37, (height() - 6) / 2, 6, 6);
    painter.setPen(Qt::NoPen);
    painter.setBrush(modifiedColor);
    painter.drawEllipse(modifiedRect);
  }

  // Close button hover rect
  QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);
  if (isCloseHovered) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 255, 255, 60)));
    painter.drawRoundedRect(closeRect.adjusted(-3, -3, 3, 3), 4, 4);
  }

  // Close button
  QPen closePen = QPen(isCloseHovered ? foregroundColor : foregroundMutedColor);
  closePen.setWidthF(1.5);

  painter.setPen(closePen);
  painter.drawLine(closeRect.topLeft() + QPoint(2, 2),
                   closeRect.bottomRight() - QPoint(2, 2));
  painter.drawLine(closeRect.topRight() + QPoint(-2, 2),
                   closeRect.bottomLeft() - QPoint(-2, 2));
}

void TabWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);
    if (closeRect.adjusted(-3, -3, 3, 3).contains(event->pos())) {
      emit closeRequested();
    } else {
      emit clicked();
    }
  } else if (event->button() == Qt::MiddleButton) {
    emit closeRequested();
  }
}

void TabWidget::enterEvent(QEnterEvent *event) {
  isHovered = true;
  update();
}

void TabWidget::mouseMoveEvent(QMouseEvent *event) {
  QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);

  if (closeRect.adjusted(-3, -3, 3, 3).contains(event->pos())) {
    isCloseHovered = true;
  } else {
    isCloseHovered = false;
  }

  update();
}

void TabWidget::leaveEvent(QEvent *event) {
  isHovered = false;
  isCloseHovered = false;
  update();
}

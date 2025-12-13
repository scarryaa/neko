#include "status_bar_widget.h"

StatusBarWidget::StatusBarWidget(neko::ConfigManager &configManager,
                                 neko::ThemeManager &themeManager,
                                 QWidget *parent)
    : QWidget(parent), configManager(configManager),
      themeManager(themeManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;
  m_height = dynamicHeight;
  setFixedHeight(m_height);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QHBoxLayout *layout = new QHBoxLayout(this);

  int leftMargin = UiUtils::getTitleBarContentMargin();

  layout->setContentsMargins(leftMargin, 5, 10, 5);
  layout->addStretch();
}

StatusBarWidget::~StatusBarWidget() {}

void StatusBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.setBrush(UiUtils::getThemeColor(themeManager, "ui.background"));
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), height())));

  painter.setPen(UiUtils::getThemeColor(themeManager, "ui.border"));
  painter.drawLine(QPointF(0, 0), QPointF(width(), 0));
}

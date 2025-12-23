#include "title_bar_widget.h"

TitleBarWidget::TitleBarWidget(neko::ConfigManager &configManager,
                               neko::ThemeManager &themeManager,
                               QWidget *parent)
    : QWidget(parent), configManager(configManager),
      themeManager(themeManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);
  m_height = dynamicHeight;
  setFixedHeight(static_cast<int>(m_height));

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  m_directorySelectionButton = new QPushButton("Select a directory");
  applyTheme();

  connect(m_directorySelectionButton, &QPushButton::clicked, this,
          &TitleBarWidget::onDirectorySelectionButtonPressed);

  auto *layout = new QHBoxLayout(this);

  int leftMargin = static_cast<int>(UiUtils::getTitleBarContentMargin());

  layout->setContentsMargins(leftMargin, VERTICAL_CONTENT_MARGIN,
                             RIGHT_CONTENT_MARGIN, VERTICAL_CONTENT_MARGIN);
  layout->addWidget(m_directorySelectionButton);
  layout->addStretch();
}

void TitleBarWidget::applyTheme() {
  if (m_directorySelectionButton == nullptr) {
    return;
  }

  QString btnText = UiUtils::getThemeColor(
      themeManager, "titlebar.button.foreground", "#a0a0a0");
  QString btnHover =
      UiUtils::getThemeColor(themeManager, "titlebar.button.hover", "#131313");
  QString btnPress = UiUtils::getThemeColor(
      themeManager, "titlebar.button.pressed", "#222222");

  m_directorySelectionButton->setStyleSheet(
      QString("QPushButton {"
              "  background-color: transparent;"
              "  color: %1;"
              "  border-radius: 6px;"
              "  padding: 4px 8px;"
              "}"
              "QPushButton:hover { background-color: %2; }"
              "QPushButton:pressed { background-color: %3; }")
          .arg(btnText, btnHover, btnPress));

  update();
}

void TitleBarWidget::onDirChanged(const std::string &newDir) {
  QString newDirQStr = QString::fromStdString(newDir);

  m_currentDir = newDirQStr;
  m_directorySelectionButton->setText(newDirQStr.split('/').last());
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.setBrush(UiUtils::getThemeColor(themeManager, "ui.background"));
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), height())));

  painter.setPen(UiUtils::getThemeColor(themeManager, "ui.border"));
  painter.drawLine(QPointF(0, height() - 1), QPointF(width(), height() - 1));
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_clickPos = event->position().toPoint();
  }

  QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons().testFlag(Qt::LeftButton)) {
    window()->move((event->globalPosition() - m_clickPos).toPoint());
    event->accept();
  }
}

void TitleBarWidget::onDirectorySelectionButtonPressed() {
  emit directorySelectionButtonPressed();
}

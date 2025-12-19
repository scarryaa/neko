#include "title_bar_widget.h"

TitleBarWidget::TitleBarWidget(neko::ConfigManager &configManager,
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

  m_directorySelectionButton = new QPushButton("Select a directory");
  applyTheme();

  connect(m_directorySelectionButton, &QPushButton::clicked, this,
          &TitleBarWidget::onDirectorySelectionButtonPressed);

  QHBoxLayout *layout = new QHBoxLayout(this);

  int leftMargin = UiUtils::getTitleBarContentMargin();

  layout->setContentsMargins(leftMargin, 5, 10, 5);
  layout->addWidget(m_directorySelectionButton);
  layout->addStretch();
}

TitleBarWidget::~TitleBarWidget() {}

void TitleBarWidget::applyTheme() {
  if (!m_directorySelectionButton) {
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

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_clickPos = event->position().toPoint();
  }

  QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    window()->move((event->globalPosition() - m_clickPos).toPoint());

    event->accept();
  }
}

void TitleBarWidget::onDirectorySelectionButtonPressed() {
  emit directorySelectionButtonPressed();
}

void TitleBarWidget::onDirChanged(std::string newDir) {
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

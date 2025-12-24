#include "title_bar_widget.h"
#include "utils/gui_utils.h"
#include <QFileInfo>

double constexpr TitleBarWidget::getPlatformTitleBarLeftInset() {
#if defined(Q_OS_MACOS)
  return MACOS_TRAFFIC_LIGHTS_INSET;
#else
  return OTHER_PLATFORMS_TRAFFIC_LIGHTS_INSET;
#endif
}

QString TitleBarWidget::getDisplayNameForDir(const QString &path) {
  QFileInfo fileInfo(path);
  const QString fileName = fileInfo.fileName();

  return fileName.isEmpty() ? path : fileName;
}

TitleBarWidget::TitleBarWidget(neko::ConfigManager &configManager,
                               neko::ThemeManager &themeManager,
                               QWidget *parent)
    : QWidget(parent), configManager(configManager),
      themeManager(themeManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setFont(uiFont);
  setFixedHeight(dynamicHeight);
  setupLayout();
  applyTheme();
}

void TitleBarWidget::directoryChanged(const std::string &newDirectoryPath) {
  auto newDirectoryPathQStr = QString::fromStdString(newDirectoryPath);
  auto displayName = TitleBarWidget::getDisplayNameForDir(newDirectoryPathQStr);
  m_directorySelectionButton->setText(displayName);
}

void TitleBarWidget::setupLayout() {
  m_directorySelectionButton = new QPushButton("Select a directory");
  connect(m_directorySelectionButton, &QPushButton::clicked, this,
          &TitleBarWidget::directorySelectionButtonPressed);

  auto *layout = new QHBoxLayout(this);
  int leftMargin =
      static_cast<int>(TitleBarWidget::getPlatformTitleBarLeftInset());

  layout->setContentsMargins(leftMargin, VERTICAL_CONTENT_MARGIN,
                             RIGHT_CONTENT_MARGIN, VERTICAL_CONTENT_MARGIN);
  layout->addWidget(m_directorySelectionButton);
  layout->addStretch();
}

void TitleBarWidget::getThemeColors() {
  auto [buttonForegroundColor, buttonHoverColor, buttonPressedColor,
        backgroundColor, borderColor] =
      UiUtils::getThemeColors(
          themeManager, "titlebar.button.foreground", "titlebar.button.hover",
          "titlebar.button.pressed", "ui.background", "ui.border");

  m_themeColors = ThemeColors{buttonForegroundColor, buttonHoverColor,
                              buttonPressedColor, backgroundColor, borderColor};
}

QString TitleBarWidget::getStyleSheet() {
  return QString("QPushButton {"
                 "  background-color: transparent;"
                 "  color: %1;"
                 "  border-radius: 6px;"
                 "  padding: 4px 8px;"
                 "}"
                 "QPushButton:hover { background-color: %2; }"
                 "QPushButton:pressed { background-color: %3; }")
      .arg(m_themeColors.buttonTextColor, m_themeColors.buttonHoverColor,
           m_themeColors.buttonPressColor);
}

void TitleBarWidget::applyTheme() {
  getThemeColors();

  if (m_directorySelectionButton != nullptr) {
    m_directorySelectionButton->setStyleSheet(getStyleSheet());
  }

  update();
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.setPen(Qt::NoPen);
  painter.setBrush(m_themeColors.backgroundColor);
  painter.drawRect(rect());

  painter.setPen(m_themeColors.borderColor);
  painter.drawLine(0, height() - 1, width(), height() - 1);
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    const QPoint localPos = event->position().toPoint();

    if (childAt(localPos) == nullptr || childAt(localPos) == this) {
      m_isDragging = true;
      m_pressGlobalPos = event->globalPosition().toPoint();
      m_windowStartPos = window()->pos();

      grabMouse();
      event->accept();
      return;
    }
  }

  QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_isDragging && ((event->buttons() & Qt::LeftButton) != 0U)) {
    const QPoint globalNow = event->globalPosition().toPoint();
    const QPoint delta = globalNow - m_pressGlobalPos;
    window()->move(m_windowStartPos + delta);

    event->accept();
    return;
  }

  QWidget::mouseMoveEvent(event);
}

void TitleBarWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && m_isDragging) {
    m_isDragging = false;
    releaseMouse();
    event->accept();
    return;
  }

  QWidget::mouseReleaseEvent(event);
}

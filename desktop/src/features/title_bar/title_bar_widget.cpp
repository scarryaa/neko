#include "title_bar_widget.h"
#include "utils/gui_utils.h"
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSizePolicy>

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

TitleBarWidget::TitleBarWidget(const TitleBarProps &props, QWidget *parent)
    : QWidget(parent), m_theme(props.theme),
      configManager(*props.configManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setFont(uiFont);
  setFixedHeight(dynamicHeight);
  setupLayout();
  setAndApplyTheme(m_theme);
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
  int leftContentInset =
      static_cast<int>(TitleBarWidget::getPlatformTitleBarLeftInset());

  layout->setContentsMargins(leftContentInset, VERTICAL_CONTENT_INSET,
                             RIGHT_CONTENT_INSET, VERTICAL_CONTENT_INSET);
  layout->addWidget(m_directorySelectionButton);
  layout->addStretch();
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
      .arg(m_theme.buttonForegroundColor, m_theme.buttonHoverColor,
           m_theme.buttonPressColor);
}

void TitleBarWidget::setAndApplyTheme(const TitleBarTheme &theme) {
  m_theme = theme;

  if (m_directorySelectionButton != nullptr) {
    m_directorySelectionButton->setStyleSheet(getStyleSheet());
  }

  update();
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.setPen(Qt::NoPen);
  painter.setBrush(m_theme.backgroundColor);
  painter.drawRect(rect());

  painter.setPen(m_theme.borderColor);
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

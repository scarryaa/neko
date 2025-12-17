#import "command_palette_widget.h"
#include "utils/gui_utils.h"

CommandPaletteWidget::CommandPaletteWidget(neko::ThemeManager &themeManager,
                                           QWidget *parent)
    : QWidget(parent), parent(parent), themeManager(themeManager) {
  setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                 Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAutoFillBackground(false);
  setFixedSize(QSize(WIDTH, HEIGHT));
  installEventFilter(this);

  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "command_palette.background");
  auto borderColor =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  auto shadowColor =
      UiUtils::getThemeColor(themeManager, "command_palette.shadow");

  QString stylesheet =
      "CommandPaletteWidget { background: transparent; border: none; "
      "} QFrame{ border-radius: 12px; background: %1; border: 1.5px "
      "solid %2; }";
  setStyleSheet(stylesheet.arg(backgroundColor, borderColor));

  QFrame *mainFrame = new QFrame(this);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                             CONTENT_MARGIN);
  layout->addWidget(mainFrame);

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(SHADOW_BLUR_RADIUS);
  shadow->setColor(shadowColor);
  shadow->setOffset(SHADOW_X_OFFSET, SHADOW_Y_OFFSET);

  mainFrame->setGraphicsEffect(shadow);
}

CommandPaletteWidget::~CommandPaletteWidget() {}

void CommandPaletteWidget::showEvent(QShowEvent *event) {
  if (!parentWidget())
    return;

  int x = (parentWidget()->width() - this->width()) / 2;
  int y = TOP_OFFSET;

  move(parentWidget()->mapToGlobal(QPoint(x, y)));

  QWidget::showEvent(event);
}

bool CommandPaletteWidget::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    if (this->geometry().contains(
            this->mapFromGlobal(mouseEvent->globalPosition().toPoint()))) {
      this->close();
      return true;
    }
  }

  return false;
}

#include "features/context_menu/context_menu_widget.h"
#include "utils/gui_utils.h"

ContextMenuWidget::ContextMenuWidget(const ContextMenuTheme &theme,
                                     neko::ConfigManager *configManager,
                                     QWidget *parent)
    : QWidget(parent), theme(theme), configManager(configManager) {
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                 Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_StyledBackground, true);
  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  setMinimumWidth(MIN_WIDTH);
  setMouseTracking(true);
  setAutoFillBackground(false);

  const auto shadowColor = theme.shadowColor;
  auto *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(SHADOW_BLUR_RADIUS);
  shadow->setColor(shadowColor);
  shadow->setOffset(SHADOW_X_OFFSET, SHADOW_Y_OFFSET);

  mainFrame =
      new ContextMenuFrame({theme.backgroundColor, theme.borderColor}, this);
  mainFrame->setGraphicsEffect(shadow);

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(SHADOW_CONTENT_MARGIN, SHADOW_CONTENT_MARGIN,
                                 SHADOW_CONTENT_MARGIN, SHADOW_CONTENT_MARGIN);
  rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
  rootLayout->addWidget(mainFrame);

  layout = new QVBoxLayout(mainFrame);
  layout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                             CONTENT_MARGIN);
  layout->setSpacing(4);

  const auto borderColor = theme.borderColor;
  const auto labelColor = theme.labelColor;
  const auto labelDisabledColor = theme.labelDisabledColor;
  const auto shortcutColor = theme.shortcutColor;
  const auto shortcutDisabledColor = theme.shortcutDisabledColor;
  const auto hoverColor = theme.hoverColor;
  const auto accentMutedColor = theme.accentMutedColor;
  const auto accentForegroundColor = theme.accentForegroundColor;

  QString styleSheet = QString(
      "QToolButton#ContextMenuItem { color: %2; background: transparent;"
      "border: none; padding: 8px 14px; text-align: left; border-radius: 6px; "
      "}"
      "QToolButton#ContextMenuItem:hover { background: %3;}"
      "QToolButton#ContextMenuItem:pressed { background: %3;}"
      "QToolButton#ContextMenuItem:disabled { color: %6; }"
      "QFrame#ContextMenuSeparator { background: %1; border: none; margin:"
      "0px; }"
      "QLabel#ContextMenuLabel { color: %7; padding: 0px 6px; }"
      "QLabel#ContextMenuLabel:disabled { color: %8; padding: 0px 6px; }"
      "QLabel#ContextMenuShortcutLabel { color: %9; padding: 0px 6px; }"
      "QLabel#ContextMenuShortcutLabel:disabled { color: %10; padding: 0px "
      "6px; }");

  setStyleSheet(styleSheet.arg(
      borderColor, labelColor, hoverColor, labelDisabledColor, labelColor,
      labelDisabledColor, shortcutColor, shortcutDisabledColor));
}

void ContextMenuWidget::showMenu(const QPoint &position) {
  const int margin = static_cast<int>(SHADOW_CONTENT_MARGIN);
  move(position - QPoint(margin / 2, margin / 2));
  show();
  setFocus(Qt::PopupFocusReason);

  qApp->installEventFilter(this);
}

void ContextMenuWidget::setItems(const QVector<ContextMenuItem> &items) {
  clearRows();

  const QFont font =
      UiUtils::loadFont(*configManager, neko::FontType::Interface);

  for (const auto &item : items) {
    if (!item.visible) {
      continue;
    }

    if (item.kind == ContextMenuItemKind::Separator) {
      auto *sep = new QFrame(mainFrame);
      sep->setFrameShape(QFrame::NoFrame);
      sep->setFrameShadow(QFrame::Plain);
      sep->setFixedHeight(1);
      sep->setObjectName("ContextMenuSeparator");
      layout->addWidget(sep);

      continue;
    }

    auto *btn = new QToolButton(mainFrame);
    btn->setObjectName("ContextMenuItem");
    btn->setAutoRaise(false);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btn->setEnabled(item.enabled);
    btn->setCheckable(true);
    btn->setChecked(item.checked);

    auto *btnLayout = new QHBoxLayout(btn);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(0);

    auto *label = new QLabel(item.label, btn);
    label->setObjectName("ContextMenuLabel");
    label->setFont(font);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setWordWrap(false);

    auto *shortcutLabel = new QLabel(item.shortcut, btn);
    shortcutLabel->setObjectName("ContextMenuShortcutLabel");
    shortcutLabel->setFont(font);
    shortcutLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    shortcutLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    shortcutLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    shortcutLabel->setVisible(!item.shortcut.isEmpty());

    btnLayout->addWidget(label);
    btnLayout->addWidget(shortcutLabel);
    btnLayout->setStretch(0, 1);
    btn->setMinimumSize(btnLayout->sizeHint());

    connect(btn, &QToolButton::clicked, this, [this, itemId = item.id] {
      emit actionTriggered(itemId);
      close();
    });

    layout->addWidget(btn);
  }

  layout->addStretch(0);
  adjustSize();
}

bool ContextMenuWidget::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    const QPoint globalPos = mouseEvent->globalPosition().toPoint();

    if (!geometry().contains(globalPos)) {
      close();
      return true;
    }
  } else if (event->type() == QEvent::ApplicationDeactivate) {
    close();
    return false;
  } else if (event->type() == QEvent::Close) {
    qApp->removeEventFilter(this);
  }

  return QWidget::eventFilter(watched, event);
}

void ContextMenuWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    close();
    return;
  }

  QWidget::keyPressEvent(event);
}

void ContextMenuWidget::clearRows() {
  while (QLayoutItem *item = layout->takeAt(0)) {
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }
}

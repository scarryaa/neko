#include "features/context_menu/context_menu_widget.h"

ContextMenuWidget::ContextMenuWidget(neko::ThemeManager *themeManager,
                                     neko::ConfigManager *configManager,
                                     QWidget *parent)
    : QWidget(parent), themeManager(themeManager),
      configManager(configManager) {
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                 Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_StyledBackground, true);
  setAttribute(Qt::WA_TranslucentBackground);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  setMinimumWidth(200);
  setMouseTracking(true);
  setAutoFillBackground(false);

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(SHADOW_BLUR_RADIUS);
  shadow->setColor(Qt::black);
  shadow->setOffset(SHADOW_X_OFFSET, SHADOW_Y_OFFSET);

  mainFrame = new ContextMenuFrame(*themeManager, this);
  mainFrame->setGraphicsEffect(shadow);

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                                 CONTENT_MARGIN);
  rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
  rootLayout->addWidget(mainFrame);

  layout = new QVBoxLayout(mainFrame);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(4);

  const auto backgroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.background");
  const auto borderColor = UiUtils::getThemeColor(*themeManager, "ui.border");
  const auto foregroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.foreground");
  const auto mutedForegroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.foreground.muted");
  const auto hoverColor =
      UiUtils::getThemeColor(*themeManager, "ui.background.hover");
  const auto accentMutedColor =
      UiUtils::getThemeColor(*themeManager, "ui.accent.muted");
  const auto accentForegroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.accent.foreground");

  QString styleSheet = QString(
      "ContextMenuWidget { background: %1; border: 1px solid %2;"
      "border-radius: 12px; }"
      "QToolButton#ContextMenuItem { color: %3; background: transparent;"
      "border: none; padding: 8px 14px; text-align: left; border-radius: 6px; "
      "}"
      "QToolButton#ContextMenuItem:hover { background: %4;}"
      "QToolButton#ContextMenuItem:pressed { background: %4;}"
      "QToolButton#ContextMenuItem:checked { background: %5; color: %6;}"
      "QToolButton#ContextMenuItem:disabled { color: %7; }"
      "QFrame#ContextMenuSeparator { background: %2; border: none; margin:"
      "0px; }"
      "QLabel#ContextMenuLabel { color: %8; padding: 0px 6px; }"
      "QLabel#ContextMenuLabel:disabled { color: %9; padding: 0px 6px; }"
      "QLabel#ContextMenuShortcutLabel { color: %10; padding: 0px 6px; }"
      "QLabel#ContextMenuShortcutLabel:disabled { color: %11; padding: 0px "
      "6px; }");

  setStyleSheet(styleSheet.arg(backgroundColor, borderColor, foregroundColor,
                               hoverColor, accentMutedColor,
                               accentForegroundColor, mutedForegroundColor,
                               foregroundColor, mutedForegroundColor,
                               mutedForegroundColor, mutedForegroundColor));
}

ContextMenuWidget::~ContextMenuWidget() {}

void ContextMenuWidget::showMenu(const QPoint &position) {
  move(position);
  show();

  qApp->installEventFilter(this);
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

void ContextMenuWidget::setItems(const QVector<ContextMenuItem> &items) {
  clearRows();

  const QFont font =
      UiUtils::loadFont(*configManager, neko::FontType::Interface);

  for (const auto &it : items) {
    if (!it.visible)
      continue;

    if (it.kind == ContextMenuItemKind::Separator) {
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
    btn->setEnabled(it.enabled);
    btn->setCheckable(true);
    btn->setChecked(it.checked);

    auto *btnLayout = new QHBoxLayout(btn);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(0);

    auto *label = new QLabel(it.label, btn);
    label->setObjectName("ContextMenuLabel");
    label->setFont(font);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setWordWrap(false);

    auto *shortcutLabel = new QLabel(it.shortcut, btn);
    shortcutLabel->setObjectName("ContextMenuShortcutLabel");
    shortcutLabel->setFont(font);
    shortcutLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    shortcutLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    shortcutLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    shortcutLabel->setVisible(!it.shortcut.isEmpty());

    btnLayout->addWidget(label);
    btnLayout->addWidget(shortcutLabel);
    btnLayout->setStretch(0, 1);
    btn->setMinimumSize(btnLayout->sizeHint());

    connect(btn, &QToolButton::clicked, this, [this, id = it.id] {
      emit actionTriggered(id);
      close();
    });

    layout->addWidget(btn);
  }

  layout->addStretch(0);
  adjustSize();
}

void ContextMenuWidget::clearRows() {
  while (QLayoutItem *item = layout->takeAt(0)) {
    if (QWidget *w = item->widget())
      w->deleteLater();
    delete item;
  }
}

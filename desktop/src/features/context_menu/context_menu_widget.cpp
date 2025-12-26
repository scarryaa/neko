#include "features/context_menu/context_menu_widget.h"
#include "theme/theme_provider.h"
#include "utils/mac_utils.h"
#include "utils/ui_utils.h"
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>

namespace g {
QPointer<ContextMenuWidget> currentMenu;
}

namespace k {
static constexpr double shadowXOffset = 0.0;
static constexpr double shadowYOffset = 5.0;
static constexpr double shadowBlurRadius = 25.0;
static constexpr double shadowContentMargin =
    20.0; // Content margin for drop shadow
static constexpr double minWidth = 200.0;
static constexpr double contentMargin = 6.0;
static constexpr double marginAdjustmentDivisor = 1.5;
} // namespace k

ContextMenuWidget::ContextMenuWidget(const ContextMenuProps &props,
                                     QWidget *parent)
    : QWidget(parent), configManager(props.configManager),
      themeProvider(props.themeProvider) {
  setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                 Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_StyledBackground, false);
  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  setMinimumWidth(k::minWidth);
  setMouseTracking(true);
  setAutoFillBackground(false);

  mainFrame =
      new ContextMenuFrame({.theme = {.backgroundColor = theme.backgroundColor,
                                      .borderColor = theme.borderColor}},
                           this);

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(k::shadowContentMargin, k::shadowContentMargin,
                                 k::shadowContentMargin,
                                 k::shadowContentMargin);
  rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
  rootLayout->addWidget(mainFrame);

  layout = new QVBoxLayout(mainFrame);
  layout->setContentsMargins(k::contentMargin, k::contentMargin,
                             k::contentMargin, k::contentMargin);
  layout->setSpacing(4);

  setAndApplyTheme(themeProvider->getContextMenuTheme());
  connectSignals();
}

ContextMenuWidget::~ContextMenuWidget() {
  if (g::currentMenu == this) {
    g::currentMenu = nullptr;
  }

  qApp->removeEventFilter(this);
}

void ContextMenuWidget::connectSignals() {
  connect(themeProvider, &ThemeProvider::contextMenuThemeChanged, this,
          &ContextMenuWidget::setAndApplyTheme);
}

void ContextMenuWidget::setAndApplyTheme(const ContextMenuTheme &newTheme) {
  theme = newTheme;
  mainFrame->setAndApplyTheme({theme.backgroundColor, theme.borderColor});

  auto *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(k::shadowBlurRadius);
  shadow->setColor(theme.shadowColor);
  shadow->setOffset(k::shadowXOffset, k::shadowYOffset);
  mainFrame->setGraphicsEffect(shadow);

  QString styleSheet = QString(
      "QToolButton#ContextMenuItem { color: %3; background: transparent;"
      "border: 0px; padding: 8px 14px; text-align: left; border-radius: 6px; "
      "}"
      "#ContextMenuItem:hover { background: %2; }"
      "#ContextMenuItem:pressed { background: %2;}"
      "#ContextMenuItem:disabled { color: %4; }"
      "#ContextMenuSeparator { background: %1; border: 0px; margin:"
      "0px; }"
      "#ContextMenuLabel { color: %3; padding: 0px 6px; background: "
      "transparent; border: 0px; }"
      "#ContextMenuLabel:disabled { color: %4; padding: 0px 6px; }"
      "#ContextMenuShortcutLabel { color: %5; padding: 0px 6px; background: "
      "transparent; border: 0px; }"
      "#ContextMenuShortcutLabel:disabled { color: %6; padding: 0px "
      "6px; }");

  setStyleSheet(styleSheet.arg(theme.borderColor)
                    .arg(theme.hoverColor)
                    .arg(theme.labelColor)
                    .arg(theme.labelDisabledColor)
                    .arg(theme.shortcutColor)
                    .arg(theme.shortcutDisabledColor));

  update();
}

void ContextMenuWidget::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);

#ifdef Q_OS_MAC
  disableWindowAnimation(static_cast<QWidget *>(this));
#endif
}

void ContextMenuWidget::showMenu(const QPoint &position) {
  // Close any previously open menu
  if (g::currentMenu && g::currentMenu != this) {
    g::currentMenu->close();
  }

  g::currentMenu = this;

  const int margin = static_cast<int>(k::shadowContentMargin);
  // Adjust for shadow margin
  int adjustedMargin = static_cast<int>(margin / k::marginAdjustmentDivisor);

  move(position - QPoint(adjustedMargin, adjustedMargin));
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
  switch (event->type()) {
  case QEvent::MouseButtonPress: {
    const auto *mouseEvent = static_cast<QMouseEvent *>(event);
    const QPoint globalPos = mouseEvent->globalPosition().toPoint();

    const int margin = static_cast<int>(k::shadowContentMargin);
    QRect clickableArea = geometry().adjusted(margin, margin, -margin, -margin);

    if (!clickableArea.contains(globalPos)) {
      close();
      return true;
    }

    break;
  }
  case QEvent::ApplicationDeactivate:
    close();
    return false;
  default:
    break;
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

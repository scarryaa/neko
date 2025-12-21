#include "features/context_menu/context_menu_widget.h"

ContextMenuWidget::ContextMenuWidget(neko::ThemeManager *themeManager,
                                     QWidget *parent)
    : QWidget(parent), themeManager(themeManager) {
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                 Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_StyledBackground, true);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  setMinimumWidth(200);
  setMouseTracking(true);
  setAutoFillBackground(false);

  layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(2);

  const auto backgroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.background");
  const auto borderColor = UiUtils::getThemeColor(*themeManager, "ui.border");

  QString styleSheet =
      QString("ContextMenuWidget { background: %1; border: 1px solid %2; }");

  setStyleSheet(styleSheet.arg(backgroundColor).arg(borderColor));
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

  for (const auto &it : items) {
    if (!it.visible)
      continue;

    if (it.kind == ContextMenuItemKind::Separator) {
      auto *sep = new QFrame(this);
      sep->setFrameShape(QFrame::HLine);
      sep->setFrameShadow(QFrame::Plain);
      sep->setFixedHeight(1);
      sep->setObjectName("ContextMenuSeparator");
      layout->addWidget(sep);

      continue;
    }

    auto *btn = new QToolButton(this);
    btn->setText(it.label);
    btn->setEnabled(it.enabled);
    btn->setCheckable(true);
    btn->setChecked(it.checked);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    btn->setAutoRaise(true);
    btn->setObjectName("ContextMenuItem");
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn->setCursor(Qt::PointingHandCursor);

    if (!it.shortcut.isEmpty())
      btn->setText(it.label + "    " + it.shortcut);

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

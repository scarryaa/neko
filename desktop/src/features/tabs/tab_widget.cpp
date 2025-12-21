#include "tab_widget.h"

TabWidget::TabWidget(const QString &title, const QString &path, int index,
                     bool isPinned, neko::ConfigManager &configManager,
                     neko::ThemeManager &themeManager,
                     ContextMenuRegistry &contextMenuRegistry,
                     CommandRegistry &commandRegistry,
                     GetTabCountFn getTabCount, QWidget *parent)
    : QWidget(parent), configManager(configManager), themeManager(themeManager),
      contextMenuRegistry(contextMenuRegistry),
      commandRegistry(commandRegistry), title(title), path(path),
      isPinned(isPinned), index(index), isActive(false),
      getTabCount_(std::move(getTabCount)) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;
  setFixedHeight(dynamicHeight);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setMouseTracking(true);

  double titleWidth = measureText(title);
  setMinimumWidth(12 + titleWidth + 44);
}

void TabWidget::setActive(bool active) {
  isActive = active;
  update();
}

void TabWidget::setModified(bool modified) {
  isModified = modified;
  update();
}

void TabWidget::setIsPinned(bool isPinned) {
  this->isPinned = isPinned;
  update();
}

bool TabWidget::getIsPinned() const { return isPinned; }

double TabWidget::measureText(QString text) {
  return fontMetrics().horizontalAdvance(text);
}

void TabWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setFont(font());

  QString borderColor = UiUtils::getThemeColor(themeManager, "ui.border");
  QString bgColor = UiUtils::getThemeColor(themeManager, "tab.inactive");
  QString hoverColor = UiUtils::getThemeColor(themeManager, "tab.hover");
  QString activeColor = UiUtils::getThemeColor(themeManager, "tab.active");
  QString foregroundColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground");
  QString foregroundMutedColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground.muted");
  QString modifiedColor = UiUtils::getThemeColor(themeManager, "ui.accent");

  // Background
  if (isActive) {
    painter.setBrush(activeColor);
  } else if (isHovered) {
    painter.setBrush(hoverColor);
  } else {
    painter.setBrush(bgColor);
  }
  painter.setPen(Qt::NoPen);
  painter.drawRect(rect());

  // Right border
  painter.setPen(QPen(borderColor));
  painter.drawLine(width() - 1, 0, width() - 1, height());

  // Bottom border
  if (!isActive) {
    painter.setPen(QPen(borderColor));
    painter.drawLine(0, height() - 1, width(), height() - 1);
  }

  // Text
  QRect textRect = rect().adjusted(12, 0, -30, 0);
  painter.setPen(isActive ? foregroundColor : foregroundMutedColor);
  painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

  // Modified marker
  if (isModified) {
    QRect modifiedRect(width() - 37, (height() - 6) / 2, 6, 6);
    painter.setPen(Qt::NoPen);
    painter.setBrush(modifiedColor);
    painter.drawEllipse(modifiedRect);
  }

  // Close button hover rect
  QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);
  if (isCloseHovered) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 255, 255, 60)));
    painter.drawRoundedRect(closeRect.adjusted(-3, -3, 3, 3), 4, 4);
  }

  // Close button
  QPen closePen = QPen(isCloseHovered ? foregroundColor : foregroundMutedColor);
  closePen.setWidthF(1.5);
  closePen.setCapStyle(Qt::RoundCap);

  if (isPinned) {
    QIcon pinIcon = QIcon::fromTheme("pin");

    if (!pinIcon.isNull()) {
      const QSize iconSize(12, 12);
      const QColor iconColor(isCloseHovered ? foregroundColor
                                            : foregroundMutedColor);
      QIcon colorizedIcon =
          UiUtils::createColorizedIcon(pinIcon, iconColor, iconSize);

      const QPixmap basePixmap = colorizedIcon.pixmap(iconSize);
      QPixmap boldPixmap(iconSize);
      boldPixmap.fill(Qt::transparent);

      QPainter iconPainter(&boldPixmap);
      iconPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      iconPainter.drawPixmap(0, 0, basePixmap);
      iconPainter.drawPixmap(-1, 0, basePixmap);
      iconPainter.drawPixmap(1, 0, basePixmap);
      iconPainter.drawPixmap(0, -1, basePixmap);
      iconPainter.drawPixmap(0, 1, basePixmap);
      iconPainter.end();

      painter.drawPixmap(closeRect.adjusted(0, 1, 0, 1).topLeft(), boldPixmap);
    }
  } else {
    painter.setPen(closePen);
    painter.drawLine(closeRect.topLeft() + QPoint(2, 2),
                     closeRect.bottomRight() - QPoint(2, 2));
    painter.drawLine(closeRect.topRight() + QPoint(-2, 2),
                     closeRect.bottomLeft() - QPoint(-2, 2));
  }
}

void TabWidget::mousePressEvent(QMouseEvent *event) {
  auto modifiers = event->modifiers();

  const bool shiftPressed =
      modifiers.testFlag(Qt::KeyboardModifier::ShiftModifier);

  if (event->button() == Qt::LeftButton) {
    QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);

    if (closeRect.adjusted(-3, -3, 3, 3).contains(event->pos())) {
      dragEligible = false;
      dragInProgress = false;

      if (isPinned) {
        emit unpinRequested();
        return;
      }

      emit closeRequested(shiftPressed);
    } else {
      dragStartPosition = event->pos();
      dragEligible = true;
      dragInProgress = false;
    }
  } else if (event->button() == Qt::MiddleButton) {
    dragEligible = false;
    dragInProgress = false;

    if (isPinned)
      return;

    emit closeRequested(shiftPressed);
  }
}

void TabWidget::enterEvent(QEnterEvent *event) {
  isHovered = true;
  update();
}

void TabWidget::mouseMoveEvent(QMouseEvent *event) {
  if ((event->buttons() & Qt::LeftButton) && dragEligible) {
    const int dragDistance =
        (event->pos() - dragStartPosition).manhattanLength();

    if (dragDistance >= QApplication::startDragDistance()) {
      dragEligible = false;
      dragInProgress = true;

      auto *drag = new QDrag(this);
      auto *mimeData = new QMimeData();
      mimeData->setData("application/x-neko-tab-index",
                        QByteArray::number(index));
      drag->setMimeData(mimeData);

      QPixmap dragPixmap = grab();
      drag->setPixmap(dragPixmap);
      drag->setHotSpot(dragStartPosition);
      drag->exec(Qt::MoveAction);

      return;
    }
  }

  QRect closeRect(width() - 24, (height() - 12) / 2, 12, 12);

  if (closeRect.adjusted(-3, -3, 3, 3).contains(event->pos())) {
    isCloseHovered = true;
  } else {
    isCloseHovered = false;
  }

  update();
}

void TabWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && dragEligible && !dragInProgress) {
    emit clicked();
  }

  dragEligible = false;
  dragInProgress = false;
}

void TabWidget::leaveEvent(QEvent *event) {
  isHovered = false;
  isCloseHovered = false;
  update();
}

void TabWidget::contextMenuEvent(QContextMenuEvent *event) {
  const int tabIndex = index;

  TabContext ctx{};
  ctx.tabIndex = tabIndex;
  ctx.isPinned = isPinned;
  ctx.isModified = isModified;
  ctx.filePath = path;
  ctx.canCloseOthers = getTabCount_() > 1;

  const QVariant v = QVariant::fromValue(ctx);
  const auto items = contextMenuRegistry.build("tab", v);

  auto *menu = new ContextMenuWidget(&themeManager, &configManager);
  menu->setItems(items);

  connect(menu, &ContextMenuWidget::actionTriggered, this,
          [this, v](const QString &id) { commandRegistry.run(id, v); });

  menu->showMenu(event->globalPos());
  event->accept();
}

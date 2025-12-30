#include "tab_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/context_menu/context_menu_widget.h"
#include "utils/ui_utils.h"
#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QDrag>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QVariant>
#include <neko-core/src/ffi/bridge.rs.h>

TabWidget::TabWidget(const TabProps &props, QWidget *parent)
    : QWidget(parent), font(props.font), themeProvider(props.themeProvider),
      theme(props.theme), contextMenuRegistry(*props.contextMenuRegistry),
      commandRegistry(*props.commandRegistry), title(props.title),
      path(props.path), isPinned(props.isPinned), index(props.index),
      tabId(props.tabId), isActive(false) {
  setFont(font);

  QFontMetrics fontMetrics(font);
  const int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);
  setFixedHeight(dynamicHeight);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setMouseTracking(true);

  const double titleWidth = measureText(title);
  setMinimumWidth(LEFT_PADDING_PX + static_cast<int>(titleWidth) +
                  MIN_RIGHT_EXTRA_PX);

  setAndApplyTheme(theme);
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

bool TabWidget::getIsModified() const { return isModified; }

QString TabWidget::getPath() const { return path; }

QString TabWidget::getTitle() const { return title; }

int TabWidget::getId() const { return tabId; }

void TabWidget::setIndex(int newIndex) { index = newIndex; }

void TabWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setFont(font);

  const QRect widgetRect = rect();

  const auto &borderColor = theme.borderColor;
  const auto &backgroundColor = theme.tabInactiveColor;
  const auto &hoverColor = theme.tabHoverColor;
  const auto &activeColor = theme.tabActiveColor;
  const auto &foregroundColor = theme.tabForegroundColor;
  const auto &foregroundMutedColor = theme.tabForegroundInactiveColor;
  const auto &modifiedColor = theme.tabModifiedIndicatorColor;
  const auto &closeHoverBackgroundColor = theme.tabCloseButtonHoverColor;

  // Background
  if (isActive) {
    painter.setBrush(activeColor);
  } else if (isHovered) {
    painter.setBrush(hoverColor);
  } else {
    painter.setBrush(backgroundColor);
  }
  painter.setPen(Qt::NoPen);
  painter.drawRect(widgetRect);

  // Right border
  painter.setPen(QPen(borderColor));
  painter.drawLine(widgetRect.right(), widgetRect.top(), widgetRect.right(),
                   widgetRect.bottom());

  // Bottom border
  if (!isActive) {
    painter.drawLine(widgetRect.left(), widgetRect.bottom(), widgetRect.right(),
                     widgetRect.bottom());
  }

  // Text
  painter.setPen(isActive ? foregroundColor : foregroundMutedColor);
  painter.drawText(titleRect(), Qt::AlignLeft | Qt::AlignVCenter, title);

  // Modified marker
  if (isModified) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(modifiedColor);
    painter.drawEllipse(modifiedRect());
  }

  const QRect cRect = closeRect();

  // Close hover background
  if (isCloseHovered) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(closeHoverBackgroundColor);
    painter.drawRoundedRect(closeHitRect(), 4, 4);
  }

  // Close button / pin glyph
  QPen closePen(isCloseHovered ? foregroundColor : foregroundMutedColor);
  closePen.setWidthF(CLOSE_PEN_THICKNESS);
  closePen.setCapStyle(Qt::RoundCap);

  if (isPinned) {
    QIcon pinIcon = QIcon::fromTheme("pin");
    if (!pinIcon.isNull()) {
      const QSize iconSize(PIN_ICON_SIZE_PX, PIN_ICON_SIZE_PX);
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

      painter.drawPixmap(
          cRect.adjusted(0, PIN_ICON_NUDGE_Y_PX, 0, PIN_ICON_NUDGE_Y_PX)
              .topLeft(),
          boldPixmap);
    }
  } else {
    painter.setPen(closePen);
    painter.drawLine(cRect.topLeft() +
                         QPoint(CLOSE_GLYPH_INSET_PX, CLOSE_GLYPH_INSET_PX),
                     cRect.bottomRight() -
                         QPoint(CLOSE_GLYPH_INSET_PX, CLOSE_GLYPH_INSET_PX));
    painter.drawLine(cRect.topRight() +
                         QPoint(-CLOSE_GLYPH_INSET_PX, CLOSE_GLYPH_INSET_PX),
                     cRect.bottomLeft() +
                         QPoint(CLOSE_GLYPH_INSET_PX, -CLOSE_GLYPH_INSET_PX));
  }
}

void TabWidget::mousePressEvent(QMouseEvent *event) {
  const auto modifiers = event->modifiers();
  const bool shiftHeld = modifiers.testFlag(Qt::ShiftModifier);

  if (event->button() == Qt::LeftButton) {
    if (closeHitRect().contains(event->pos())) {
      dragEligible = false;
      dragInProgress = false;

      if (isPinned) {
        emit unpinRequested(tabId);
      } else {
        emit closeRequested(neko::CloseTabOperationTypeFfi::Single, tabId,
                            shiftHeld);
      }
      event->accept();
      return;
    }

    dragStartPosition = event->pos();
    dragEligible = true;
    dragInProgress = false;
    event->accept();
    return;
  }

  if (event->button() == Qt::MiddleButton) {
    dragEligible = false;
    dragInProgress = false;
    middleClickPending = true;
    event->accept();
    return;
  }

  QWidget::mousePressEvent(event);
}

void TabWidget::mouseMoveEvent(QMouseEvent *event) {
  if (((event->buttons() & Qt::LeftButton) != 0U) && dragEligible) {
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

      const QPixmap dragPixmap = grab();
      drag->setPixmap(dragPixmap);
      drag->setHotSpot(dragStartPosition);
      drag->exec(Qt::MoveAction);
      return;
    }
  }

  const bool wasCloseHovered = isCloseHovered;
  isCloseHovered = closeHitRect().contains(event->pos());

  if (isCloseHovered != wasCloseHovered) {
    update(closeHitRect());
  }
}

void TabWidget::mouseReleaseEvent(QMouseEvent *event) {
  const auto modifiers = event->modifiers();
  const bool shiftPressed = modifiers.testFlag(Qt::ShiftModifier);

  if (event->button() == Qt::LeftButton && dragEligible && !dragInProgress) {
    emit clicked(tabId);
    event->accept();
  } else if (event->button() == Qt::MiddleButton) {
    if (middleClickPending) {
      middleClickPending = false;

      if (!isPinned && rect().contains(event->pos())) {
        emit closeRequested(neko::CloseTabOperationTypeFfi::Single, tabId,
                            shiftPressed);
      }
    }

    event->accept();
  }

  dragEligible = false;
  dragInProgress = false;

  QWidget::mouseReleaseEvent(event);
}

void TabWidget::enterEvent(QEnterEvent *event) {
  if (!isHovered) {
    isHovered = true;
    update();
  }
}

void TabWidget::leaveEvent(QEvent *event) {
  QWidget::leaveEvent(event);

  bool needsUpdate = false;

  if (isHovered) {
    isHovered = false;
    needsUpdate = true;
  }

  if (isCloseHovered) {
    isCloseHovered = false;
    needsUpdate = true;
  }

  if (needsUpdate) {
    update();
  }
}

double TabWidget::measureText(const QString &text) {
  QFontMetrics fontMetrics(font);
  return static_cast<double>(fontMetrics.horizontalAdvance(text));
}

QRect TabWidget::closeRect() const {
  const int yPos = (height() - CLOSE_BUTTON_SIZE_PX) / 2;
  const int xPos = width() - CLOSE_BUTTON_RIGHT_INSET_PX;
  return {xPos, yPos, CLOSE_BUTTON_SIZE_PX, CLOSE_BUTTON_SIZE_PX};
}

QRect TabWidget::closeHitRect() const {
  return closeRect().adjusted(-CLOSE_HIT_INFLATE_PX, -CLOSE_HIT_INFLATE_PX,
                              CLOSE_HIT_INFLATE_PX, CLOSE_HIT_INFLATE_PX);
}

QRect TabWidget::titleRect() const {
  return rect().adjusted(LEFT_PADDING_PX, 0, -RIGHT_RESERVED_FOR_CONTROLS_PX,
                         0);
}

QRect TabWidget::modifiedRect() const {
  const int yPos = (height() - MODIFIED_DOT_SIZE_PX) / 2;
  const int xPos = width() - MODIFIED_DOT_RIGHT_INSET_PX;
  return {xPos, yPos, MODIFIED_DOT_SIZE_PX, MODIFIED_DOT_SIZE_PX};
}

void TabWidget::contextMenuEvent(QContextMenuEvent *event) {
  // TODO(scarlet): Change from neko:: type
  neko::TabContextFfi ctx{static_cast<uint64_t>(tabId), isPinned, isModified,
                          !path.isEmpty(), path.toStdString()};
  const QVariant variant = QVariant::fromValue(ctx);
  const auto items = contextMenuRegistry.build("tab", variant);

  auto *menu = new ContextMenuWidget(
      {.themeProvider = themeProvider, .font = font}, nullptr);
  menu->setItems(items);

  connect(menu, &ContextMenuWidget::actionTriggered, this,
          [this, variant](const QString &actionId) {
            commandRegistry.run(actionId, variant);
          });

  menu->showMenu(event->globalPos());
  event->accept();
}

void TabWidget::setAndApplyTheme(const TabTheme &newTheme) {
  theme = newTheme;
  update();
}

void TabWidget::setTitle(const QString &newTitle) {
  title = newTitle;

  const double titleWidth = measureText(title);
  setMinimumWidth(LEFT_PADDING_PX + static_cast<int>(titleWidth) +
                  MIN_RIGHT_EXTRA_PX);
}

void TabWidget::setPath(const QString &newPath) { path = newPath; }

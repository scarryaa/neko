#include "tab_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/context_menu/providers/tab_context.h"
#include "theme/theme_provider.h"
#include "utils/gui_utils.h"
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
#include <utility>

// NOLINTNEXTLINE
TabWidget::TabWidget(const QString &title, QString path, int index, int tabId,
                     bool isPinned, neko::ConfigManager &configManager,
                     ThemeProvider *themeProvider, const TabTheme &theme,
                     ContextMenuRegistry &contextMenuRegistry,
                     CommandRegistry &commandRegistry,
                     GetTabCountFn getTabCount, QWidget *parent)
    : QWidget(parent), configManager(configManager),
      themeProvider(themeProvider), theme(theme),
      contextMenuRegistry(contextMenuRegistry),
      commandRegistry(commandRegistry), title(title), path(std::move(path)),
      isPinned(isPinned), index(index), tabId(tabId), isActive(false),
      getTabCount_(std::move(getTabCount)) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);
  setFixedHeight(dynamicHeight);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setMouseTracking(true);

  double titleWidth = measureText(title);
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

int TabWidget::getId() const { return tabId; }

void TabWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setFont(font());

  QString borderColor = theme.borderColor;
  QString bgColor = theme.tabInactiveColor;
  QString hoverColor = theme.tabHoverColor;
  QString activeColor = theme.tabActiveColor;
  QString foregroundColor = theme.tabForegroundColor;
  QString foregroundMutedColor = theme.tabForegroundInactiveColor;
  QString modifiedColor = theme.tabModifiedIndicatorColor;

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
  painter.setPen(isActive ? foregroundColor : foregroundMutedColor);
  painter.drawText(titleRect(), Qt::AlignLeft | Qt::AlignVCenter, title);

  // Modified marker
  if (isModified) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(modifiedColor);
    painter.drawEllipse(modifiedRect());
  }

  const QRect cRect = closeRect();
  if (isCloseHovered) {
    painter.setPen(Qt::NoPen);
    auto closeHoverColor = theme.tabCloseButtonHoverColor;
    painter.setBrush(closeHoverColor);
    painter.drawRoundedRect(closeHitRect(), 4, 4);
  }

  // Close button glyph
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
  auto modifiers = event->modifiers();

  const bool shiftPressed =
      modifiers.testFlag(Qt::KeyboardModifier::ShiftModifier);

  if (event->button() == Qt::LeftButton) {
    if (closeHitRect().contains(event->pos())) {
      dragEligible = false;
      dragInProgress = false;

      if (isPinned) {
        emit unpinRequested();
        return;
      }

      emit closeRequested(shiftPressed);
      return;
    }

    dragStartPosition = event->pos();
    dragEligible = true;
    dragInProgress = false;
  } else if (event->button() == Qt::MiddleButton) {
    dragEligible = false;
    dragInProgress = false;

    if (isPinned) {
      return;
    }

    emit closeRequested(shiftPressed);
  }
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

      QPixmap dragPixmap = grab();
      drag->setPixmap(dragPixmap);
      drag->setHotSpot(dragStartPosition);
      drag->exec(Qt::MoveAction);

      return;
    }
  }

  isCloseHovered = closeHitRect().contains(event->pos());
  update();
}

void TabWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && dragEligible && !dragInProgress) {
    emit clicked();
  }

  dragEligible = false;
  dragInProgress = false;
}

void TabWidget::enterEvent(QEnterEvent *event) {
  isHovered = true;
  update();
}

void TabWidget::leaveEvent(QEvent *event) {
  isHovered = false;
  isCloseHovered = false;
  update();
}

double TabWidget::measureText(const QString &text) {
  return fontMetrics().horizontalAdvance(text);
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
  // Left padding, reserve right side for close/modifier area
  return rect().adjusted(LEFT_PADDING_PX, 0, -RIGHT_RESERVED_FOR_CONTROLS_PX,
                         0);
}

QRect TabWidget::modifiedRect() const {
  const int yPos = (height() - MODIFIED_DOT_SIZE_PX) / 2;
  const int xPos = width() - MODIFIED_DOT_RIGHT_INSET_PX;
  return {xPos, yPos, MODIFIED_DOT_SIZE_PX, MODIFIED_DOT_SIZE_PX};
}

void TabWidget::contextMenuEvent(QContextMenuEvent *event) {
  TabContext ctx{};
  ctx.tabId = tabId;
  ctx.isPinned = isPinned;
  ctx.isModified = isModified;
  ctx.filePath = path;
  ctx.canCloseOthers = getTabCount_() > 1;

  const QVariant variant = QVariant::fromValue(ctx);
  const auto items = contextMenuRegistry.build("tab", variant);
  auto *menu = new ContextMenuWidget(themeProvider, &configManager, this);

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

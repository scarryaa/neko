#include "file_explorer_widget.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/file_explorer/controllers/file_explorer_controller.h"
#include "features/main_window/services/dialog_service.h"
#include "utils/ui_utils.h"
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtDebug>

// TODO(scarlet): Allow inline renaming/item creation via an input field, rather
// than showing a dialog.
FileExplorerWidget::FileExplorerWidget(const FileExplorerProps &props,
                                       QWidget *parent)
    : QScrollArea(parent), font(props.font), theme(props.theme),
      fontMetrics(font), contextMenuRegistry(*props.contextMenuRegistry),
      commandRegistry(*props.commandRegistry),
      themeProvider(props.themeProvider),
      fileExplorerController(FileExplorerController(
          {.fileTreeBridge = props.fileTreeBridge}, parent)) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  directorySelectionButton = new QPushButton("Select a directory");

  setAndApplyTheme(theme);

  auto *layout = new QVBoxLayout();
  layout->addWidget(directorySelectionButton, 0, Qt::AlignCenter);
  setLayout(layout);

  connectSignals();
}

void FileExplorerWidget::connectSignals() {
  connect(directorySelectionButton, &QPushButton::clicked, this,
          [this]() { emit directorySelectionRequested(); });
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          &FileExplorerWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &FileExplorerWidget::redraw);
}

void FileExplorerWidget::setAndApplyTheme(const FileExplorerTheme &newTheme) {
  theme = newTheme;

  setStyleSheet(UiUtils::getScrollBarStylesheet(
      theme.scrollBarTheme.thumbColor, theme.scrollBarTheme.thumbHoverColor,
      "FileExplorerWidget", theme.backgroundColor));

  if (directorySelectionButton != nullptr) {
    QString buttonBg = theme.buttonBackgroundColor;
    QString buttonHover = theme.buttonHoverColor;
    QString buttonPressed = theme.buttonPressColor;
    QString buttonText = theme.buttonForegroundColor;

    directorySelectionButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border-radius: "
                "6px; padding: 8px 16px; font-size: 15px; border: none; }"
                "QPushButton:hover { background-color: %3; }"
                "QPushButton:pressed { background-color: %4; }")
            .arg(buttonBg, buttonText, buttonHover, buttonPressed));
  }

  redraw();
}

void FileExplorerWidget::itemRevealRequested() {
  auto nodeInfo = fileExplorerController.getNode(
      [](const auto node) { return node.is_current; });

  if (nodeInfo.foundNode()) {
    scrollToNode(nodeInfo.index);
    redraw();
    setFocus();
  }
}

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);

  if (ctrl) {
    bool shouldRedraw = false;

    // Handle node or font operations.
    switch (event->key()) {
    case Qt::Key_C:
      fileExplorerController.handleCopy();
      return;
    case Qt::Key_V:
      shouldRedraw = true;
      fileExplorerController.handlePaste();
      break;
    case Qt::Key_X:
      fileExplorerController.handleCut();
      return;
    case Qt::Key_D:
      shouldRedraw = true;
      fileExplorerController.handleDuplicate();
      break;
    case Qt::Key_Equal:
      shouldRedraw = true;
      increaseFontSize();
      break;
    case Qt::Key_Minus:
      shouldRedraw = true;
      decreaseFontSize();
      break;
    case Qt::Key_0:
      shouldRedraw = true;
      resetFontSize();
      break;
    default:
      break;
    }

    if (shouldRedraw) {
      redraw();
      return;
    }
  }

  // Handle node/navigation operations.
  switch (event->key()) {
  case Qt::Key_Up:
    fileExplorerController.handleUp();
    shouldScroll = true;
    break;
  case Qt::Key_Down:
    fileExplorerController.handleDown();
    shouldScroll = true;
    break;
  case Qt::Key_Left:
    fileExplorerController.handleLeft();
    shouldScroll = true;
    break;
  case Qt::Key_Right:
    fileExplorerController.handleRight();
    shouldScroll = true;
    break;
  case Qt::Key_Space:
    fileExplorerController.handleSpace();
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    fileExplorerController.handleEnter();
    break;
  case Qt::Key_Delete:
    if (shift) {
      // Skip the delete confirmation dialog.
      fileExplorerController.handleDelete(false);
    } else {
      // Show the delete confirmation dialog.
      fileExplorerController.handleDelete(true);
    }
  default:
    break;
  }

  if (shouldScroll) {
    auto nodeInfo = fileExplorerController.getNode(
        [](const auto node) { return node.is_current; });

    if (nodeInfo.foundNode()) {
      scrollToNode(nodeInfo.index);
    }
  }

  redraw();
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  const bool isLeftClick = (event->button() == Qt::LeftButton);
  const bool refocusClick = focusReceivedFromMouse;

  const int row = convertMousePositionToRow(event->pos().y());
  const int nodeCount = fileExplorerController.getNodeCount();

  // Reset the focus flag.
  focusReceivedFromMouse = false;

  // Clear the selected node if click is not on a node.
  if (row < 0 || row >= nodeCount) {
    if (isLeftClick && refocusClick) {
      setFocus();
    } else {
      fileExplorerController.clearSelection();
      redraw();
    }

    return;
  }

  auto clickResult = fileExplorerController.handleNodeClick(row, isLeftClick);

  switch (clickResult.action) {
  case FileExplorerController::Action::LayoutChanged:
    // A directory was expanded or collapsed.
    updateDimensions();
    break;
  case FileExplorerController::Action::FileSelected:
    // A file was selected.
    emit fileSelected(clickResult.filePath, false);
    break;
  case FileExplorerController::Action::None:
    // Nothing changed, just redraw (done below).
    break;
  }

  redraw();
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
  auto snapshot = fileExplorerController.getTreeSnapshot();

  drawFiles(&painter, snapshot.nodes.size(), snapshot.nodes);
}

void FileExplorerWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta = (event->isInverted() ? -1 : 1) *
                         event->angleDelta().y() / SCROLL_WHEEL_DIVIDER;
  double horizontallDelta = (event->isInverted() ? -1 : 1) *
                            event->angleDelta().x() / SCROLL_WHEEL_DIVIDER;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(static_cast<int>(newHorizontalScrollOffset));
  verticalScrollBar()->setValue(static_cast<int>(newVerticalScrollOffset));
  redraw();
}

void FileExplorerWidget::resizeEvent(QResizeEvent *event) {
  updateDimensions();
}

void FileExplorerWidget::focusInEvent(QFocusEvent *event) {
  focusReceivedFromMouse = event->reason() == Qt::MouseFocusReason;
  QScrollArea::focusInEvent(event);
}

void FileExplorerWidget::focusOutEvent(QFocusEvent *event) {
  focusReceivedFromMouse = false;
  QScrollArea::focusOutEvent(event);
}

void FileExplorerWidget::applySelectedDirectory(const QString &path) {
  // If the path is empty, return.
  if (path.isEmpty()) {
    return;
  }

  fileExplorerController.loadDirectory(path);
  directorySelectionButton->hide();

  emit directorySelected(path);
  emit directoryPersistRequested(path);
}

void FileExplorerWidget::redraw() { viewport()->update(); }

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   rust::Vec<neko::FileNodeSnapshot> nodes) {
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();

  int startRow = std::floor(verticalOffset / lineHeight);
  int endRow = std::ceil((verticalOffset + viewportHeight) / lineHeight);

  startRow = std::max(0, startRow);
  endRow = std::min(static_cast<int>(count), endRow);

  for (int i = startRow; i < endRow; i++) {
    double yPos = (i * lineHeight) - verticalOffset;
    drawFile(painter, -horizontalOffset + ICON_EDGE_PADDING, yPos, nodes[i]);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double xPos, double yPos,
                                  const neko::FileNodeSnapshot &node) {
  painter->setFont(font);

  double indent = static_cast<int>(node.depth) * NODE_INDENT;
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();
  double horizontalOffset = horizontalScrollBar()->value();

  auto accentColor = theme.selectionColor;
  auto selectionColor = QColor(accentColor);
  selectionColor.setAlpha(SELECTION_ALPHA);

  auto fileTextColor = theme.fileForegroundColor;

  // Selection background
  if (node.is_selected) {
    painter->setBrush(selectionColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRectF(
        -horizontalOffset, yPos,
        viewportWidth + ICON_EDGE_PADDING + horizontalOffset, lineHeight));
  }

  // Current item border
  if (node.is_current && hasFocus()) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(accentColor);
    painter->drawRect(QRectF(-horizontalOffset, yPos,
                             viewportWidth - 1 + horizontalOffset,
                             lineHeight - 1));
  }

  // Get appropriate icon
  QIcon icon;
  if (node.is_dir) {
    icon = QApplication::style()->standardIcon(
        node.is_expanded ? QStyle::SP_DirOpenIcon : QStyle::SP_DirIcon);
  } else {
    icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
  }

  int iconSize = static_cast<int>(lineHeight - ICON_ADJUSTMENT);
  QIcon colorizedIcon = UiUtils::createColorizedIcon(icon, accentColor,
                                                     QSize(iconSize, iconSize));
  QIcon normalIcon = UiUtils::createColorizedIcon(icon, fileTextColor,
                                                  QSize(iconSize, iconSize));
  QPixmap pixmap = colorizedIcon.pixmap(iconSize, iconSize);

  if (node.is_hidden) {
    pixmap = colorizedIcon.pixmap(iconSize, iconSize, QIcon::Mode::Disabled,
                                  QIcon::State::Off);
  }

  if (!node.is_dir) {
    pixmap = normalIcon.pixmap(iconSize, iconSize);
  }

  // Draw icon with indentation
  double iconX = xPos + indent + 2;
  double iconY = yPos + ((lineHeight - iconSize) / 2);
  painter->drawPixmap(QPointF(iconX, iconY), pixmap);

  // Draw text after icon
  double textX = iconX + iconSize + 4;
  if (node.is_hidden) {
    QString foregroundVeryMuted = theme.fileHiddenColor;
    painter->setBrush(foregroundVeryMuted);
    painter->setPen(foregroundVeryMuted);
  } else {
    painter->setBrush(fileTextColor);
    painter->setPen(fileTextColor);
  }
  painter->drawText(QPointF(textX, yPos + fontMetrics.ascent()),
                    QString::fromUtf8(node.name));
}

void FileExplorerWidget::onRootDirectoryChanged() {
  updateDimensions();
  redraw();
}

double FileExplorerWidget::measureContentWidth() {
  auto snapshot = fileExplorerController.getTreeSnapshot();
  double finalWidth = 0;

  // TODO(scarlet): Make this faster if possible?
  for (auto &node : snapshot.nodes) {
    QString lineText = QString::fromUtf8(node.name.c_str());
    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::updateDimensions() {
  auto nodeCount = fileExplorerController.getNodeCount();

  const double lineHeight = fontMetrics.height();
  const double viewportHeight = std::max(
      0.0, (nodeCount * lineHeight) - viewport()->height() + VIEWPORT_PADDING);
  const double viewportWidth = std::max(
      0.0, measureContentWidth() - viewport()->width() + VIEWPORT_PADDING);

  horizontalScrollBar()->setRange(0, static_cast<int>(viewportWidth));
  verticalScrollBar()->setRange(0, static_cast<int>(viewportHeight));
}

void FileExplorerWidget::scrollToNode(int index) {
  const double viewportHeight = viewport()->height();
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();
  const double nodeY = index * lineHeight;

  if (nodeY + lineHeight > viewportHeight - VIEWPORT_PADDING + scrollY) {
    verticalScrollBar()->setValue(static_cast<int>(
        nodeY - viewportHeight + VIEWPORT_PADDING + lineHeight));
  } else if (nodeY < scrollY + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(static_cast<int>(nodeY - VIEWPORT_PADDING));
  }
}

void FileExplorerWidget::increaseFontSize() {
  if (font.pointSizeF() < FONT_UPPER_LIMIT) {
    setFontSize(font.pointSizeF() + FONT_STEP);
  }
}

void FileExplorerWidget::decreaseFontSize() {
  if (font.pointSizeF() > FONT_LOWER_LIMIT) {
    setFontSize(font.pointSizeF() - FONT_STEP);
  }
}

void FileExplorerWidget::resetFontSize() { setFontSize(DEFAULT_FONT_SIZE); }

void FileExplorerWidget::setFontSize(double newFontSize) {
  font.setPointSizeF(newFontSize);
  fontMetrics = QFontMetricsF(font);

  redraw();
  emit fontSizeChanged(newFontSize);
}

int FileExplorerWidget::convertMousePositionToRow(double yPos) {
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();

  const int targetRow = static_cast<int>((yPos + scrollY) / lineHeight);

  return targetRow;
}

void FileExplorerWidget::contextMenuEvent(QContextMenuEvent *event) {
  auto snapshot = fileExplorerController.getTreeSnapshot();
  int row = convertMousePositionToRow(event->pos().y());

  bool targetIsItem = false;
  bool itemIsDirectory = false;
  bool itemIsExpanded = false;
  QString path = QString::fromUtf8(snapshot.root.c_str());
  if (row < snapshot.nodes.size()) {
    // Click was over an item.
    const auto currentNode = snapshot.nodes[row];
    path = QString::fromUtf8(currentNode.path);
    targetIsItem = true;
    itemIsDirectory = targetIsItem && currentNode.is_dir;
    itemIsExpanded = currentNode.is_expanded;
  }

  neko::FileExplorerContextFfi ctx{.item_path = path.toStdString(),
                                   .target_is_item = targetIsItem,
                                   .item_is_directory = itemIsDirectory,
                                   .item_is_expanded = itemIsExpanded};

  const QVariant variant = QVariant::fromValue(ctx);
  const auto items = contextMenuRegistry.build("fileExplorer", variant);

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

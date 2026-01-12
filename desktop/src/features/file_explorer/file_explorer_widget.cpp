#include "file_explorer_widget.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/file_explorer/controllers/file_explorer_controller.h"
#include "features/file_explorer/render/file_explorer_renderer.h"
#include "features/main_window/services/dialog_service.h"
#include "utils/ui_utils.h"
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QDrag>
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
      fileExplorerController(new FileExplorerController(
          {.fileTreeBridge = props.fileTreeBridge}, this)) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);
  setMouseTracking(true);
  setAcceptDrops(true);

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
  auto nodeInfo = fileExplorerController->getNode(
      [](const auto node) { return node.is_current; });

  if (nodeInfo.foundNode()) {
    scrollToNode(nodeInfo.index);
    redraw();
    setFocus();
  }
}

// TODO(scarlet): Add customizable keybindings/vim keybinds.
// TODO(scarlet): Convert the switches to a map?
void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);
  const bool alt = mods.testFlag(Qt::AltModifier);

  if (ctrl) {
    bool shouldRedraw = false;

    // Handle node or font operations.
    switch (event->key()) {
    case Qt::Key_C:
      if (alt) {
        if (shift) {
          triggerCommand("fileExplorer.copyRelativePath");
        } else {
          triggerCommand("fileExplorer.copyPath");
        }
      } else {
        fileExplorerController->handleCopy();
      }
      return;
    case Qt::Key_V:
      shouldRedraw = true;
      triggerCommand("fileExplorer.paste");
      break;
    case Qt::Key_X:
      triggerCommand("fileExplorer.cut");
      return;
    case Qt::Key_D:
      shouldRedraw = true;
      triggerCommand("fileExplorer.duplicate");
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
    case Qt::Key_Backslash:
      triggerCommand("fileExplorer.findInFolder");
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
    triggerCommand("fileExplorer.navigateUp");
    shouldScroll = true;
    break;
  case Qt::Key_Down:
    triggerCommand("fileExplorer.navigateDown");
    shouldScroll = true;
    break;
  case Qt::Key_Left:
    triggerCommand("fileExplorer.navigateLeft");
    shouldScroll = true;
    break;
  case Qt::Key_Right:
    triggerCommand("fileExplorer.navigateRight");
    shouldScroll = true;
    break;
  case Qt::Key_Space:
    // Toggle select for this node.
    // TODO(scarlet): Add support operations on multiple nodes.
    triggerCommand("fileExplorer.toggleSelect");
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
  case Qt::Key_E:
    // Toggle expand/collapse for this node if it's a directory, or open it if
    // it's a file.
    emit requestFocusEditor(true);
    triggerCommand("fileExplorer.action");
    break;
  case Qt::Key_Delete:
    if (shift) {
      // Delete and skip the delete confirmation dialog.
      triggerCommand("fileExplorer.delete", true);
    } else {
      // Delete, but show the delete confirmation dialog.
      triggerCommand("fileExplorer.delete");
    }
    break;
  case Qt::Key_R:
    if (shift) {
      triggerCommand("fileExplorer.rename");
    }
    break;
  case Qt::Key_X:
    triggerCommand("fileExplorer.reveal");
    break;
  case Qt::Key_D:
    if (shift) {
      // Delete, but show the delete confirmation dialog.
      triggerCommand("fileExplorer.delete");
    } else {
      triggerCommand("fileExplorer.newFolder");
    }
    break;
  case Qt::Key_Percent:
    triggerCommand("fileExplorer.newFile");
    break;
  case Qt::Key_C:
    if (shift) {
      triggerCommand("fileExplorer.collapseAll");
    }
  case Qt::Key_Escape:
    triggerCommand("fileExplorer.clearSelected");
  default:
    break;
  }

  if (shouldScroll) {
    auto nodeInfo = fileExplorerController->getNode(
        [](const auto node) { return node.is_current; });

    if (nodeInfo.foundNode()) {
      scrollToNode(nodeInfo.index);
    }
  }

  redraw();
}

void FileExplorerWidget::triggerCommand(
    const std::string &commandId, bool bypassDeleteConfirmation, int index,
    const std::string &targetNodePath, const std::string &destinationNodePath) {
  // Retreive the context.
  auto ctx = fileExplorerController->getCurrentContext();
  ctx.index = index;
  ctx.move_target_node_path = targetNodePath;
  ctx.move_destination_node_path = destinationNodePath;

  emit commandRequested(commandId, ctx, bypassDeleteConfirmation);
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }

  dragStartPosition = event->pos();
  event->accept();
}

void FileExplorerWidget::mouseMoveEvent(QMouseEvent *event) {
  const int row = convertMousePositionToRow(event->pos().y());
  const auto targetNode = fileExplorerController->getNodeByIndex(row);

  // Set the hovered node path for the hover effect.
  if (targetNode.foundNode()) {
    hoveredNodePath = QString::fromUtf8(targetNode.nodeSnapshot.path);
  } else {
    hoveredNodePath = nullptr;
  }

  // If the left mouse button is held, start a drag.
  if (((event->buttons() & Qt::LeftButton) != 0U) && !isDragging) {
    if ((event->pos() - dragStartPosition).manhattanLength() >=
        QApplication::startDragDistance()) {

      if (targetNode.foundNode()) {
        isDragging = true;
        draggedNode = targetNode.nodeSnapshot;

        performDrag(row, targetNode.nodeSnapshot);

        isDragging = false;
        hoveredNodePath = nullptr;
        redraw();
      }
    }
  }

  event->accept();
  redraw();
}

void FileExplorerWidget::mouseReleaseEvent(QMouseEvent *event) {
  const int row = convertMousePositionToRow(event->pos().y());
  const auto targetNode = fileExplorerController->getNodeByIndex(row);

  // If the click was not the left mouse button, only select the target node.
  if (event->button() != Qt::LeftButton) {
    fileExplorerController->setCurrent(
        QString::fromUtf8(targetNode.nodeSnapshot.path));

    return;
  }

  event->accept();

  // Trigger an action but do NOT focus the editor (if opening a file).
  triggerCommand("fileExplorer.actionIndex", false, row);
  redraw();
}

void FileExplorerWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  // If the click was not the left mouse button, don't do anything.
  if (event->button() != Qt::LeftButton) {
    return;
  }

  const int row = convertMousePositionToRow(event->pos().y());

  // Trigger an action AND focus the editor (if opening a file).
  emit requestFocusEditor(true);
  triggerCommand("fileExplorer.actionIndex", false, row);
  redraw();
}

void FileExplorerWidget::performDrag(int row,
                                     const neko::FileNodeSnapshot &node) {
  auto *drag = new QDrag(this);
  auto *mimeData = new QMimeData();

  mimeData->setData("application/x-neko-file-explorer-node",
                    QByteArray::fromStdString(std::string(node.path)));
  drag->setMimeData(mimeData);

  double lineHeight = fontMetrics.height();
  int verticalOffset = verticalScrollBar()->value();
  double nodeNameWidth = measureFileNameWidth(QString::fromUtf8(node.name));
  int iconSize = static_cast<int>(lineHeight - ICON_ADJUSTMENT);
  double iconX = ICON_EDGE_PADDING + 2;
  double textX = iconX + iconSize + 4;
  double yPos = (row * lineHeight) - verticalOffset;
  double width =
      EDGE_INSET + iconSize + ICON_SPACING + nodeNameWidth + EDGE_INSET;

  QPixmap dragPixmap(static_cast<int>(width),
                     static_cast<int>(lineHeight * 1.5));
  dragPixmap.fill(Qt::transparent);

  {
    auto ctx = getViewportContext();
    auto state = getRenderState();

    QPainter painter(&dragPixmap);
    painter.setOpacity(0.8);
    FileExplorerRenderer::drawDragGhost(painter, state, ctx, row);
  }

  QPixmap fakePixmap(1, 1);
  fakePixmap.fill(Qt::transparent);

  {
    QPainter painter(&fakePixmap);
    painter.setOpacity(0);
  }

  drag->setPixmap(fakePixmap);
  drag->setDragCursor(dragPixmap, Qt::MoveAction);
  drag->setDragCursor(dragPixmap, Qt::CopyAction);
  drag->setDragCursor(dragPixmap, Qt::LinkAction);
  drag->setDragCursor(dragPixmap, Qt::IgnoreAction);

  drag->exec(Qt::MoveAction);
}

void FileExplorerWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasFormat("application/x-neko-file-explorer-node")) {
    event->acceptProposedAction();
  }
}

void FileExplorerWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasFormat("application/x-neko-file-explorer-node")) {
    // TODO(scarlet): Auto expand directories if hovering over them for a few
    // seconds.
    int row = convertMousePositionToRow(event->position().y());
    const auto targetNode = fileExplorerController->getNodeByIndex(row);

    if (targetNode.foundNode()) {
      hoveredNodePath = QString::fromUtf8(targetNode.nodeSnapshot.path);
    } else {
      hoveredNodePath = nullptr;
    }
    redraw();

    event->acceptProposedAction();
  }
}

void FileExplorerWidget::dropEvent(QDropEvent *event) {
  const int row = convertMousePositionToRow(event->position().y());
  const auto targetNode = fileExplorerController->getNodeByIndex(row);

  const QByteArray encodedData =
      event->mimeData()->data("application/x-neko-file-explorer-node");
  const std::string sourcePath = encodedData.toStdString();

  // Pass an empty string to allow for moving an item to the root directory.
  const std::string destinationPath =
      targetNode.foundNode() ? std::string(targetNode.nodeSnapshot.path) : "";

  triggerCommand("fileExplorer.move", false, -1, sourcePath, destinationPath);

  event->acceptProposedAction();
}

FileExplorerRenderState FileExplorerWidget::getRenderState() {
  auto snapshot = fileExplorerController->getTreeSnapshot();

  FileExplorerRenderState state{
      .snapshot = snapshot,
      .font = font,
      .fontAscent = fontMetrics.ascent(),
      .theme = theme,
      .hasFocus = hasFocus(),
      .hoveredNodePath = hoveredNodePath,
      .measureFileNameWidth = [this](const QString &string) {
        return fontMetrics.horizontalAdvance(string);
      }};

  return state;
}

FileExplorerViewportContext FileExplorerWidget::getViewportContext() {
  auto snapshot = fileExplorerController->getTreeSnapshot();

  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double viewportWidth = viewport()->width();

  int startRow = std::floor(verticalOffset / lineHeight);
  int endRow = std::ceil((verticalOffset + viewportHeight) / lineHeight);

  startRow = std::max(0, startRow);
  endRow = std::min(static_cast<int>(snapshot.nodes.size()), endRow);

  FileExplorerViewportContext ctx{
      .lineHeight = lineHeight,
      .firstVisibleLine = startRow,
      .lastVisibleLine = endRow,
      .verticalOffset = verticalOffset,
      .horizontalOffset = horizontalOffset,
      .width = viewportWidth,
      .height = viewportHeight,
  };

  return ctx;
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  auto state = getRenderState();
  auto ctx = getViewportContext();

  FileExplorerRenderer::paint(painter, state, ctx);
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

void FileExplorerWidget::applySelectedDirectory(const QString &path) {
  // If the path is empty, return.
  if (path.isEmpty()) {
    return;
  }

  fileExplorerController->loadDirectory(path);
  directorySelectionButton->hide();

  emit directorySelected(path);
  emit directoryPersistRequested(path);
}

void FileExplorerWidget::redraw() { viewport()->update(); }

void FileExplorerWidget::onRootDirectoryChanged() {
  updateDimensions();
  redraw();
}

double FileExplorerWidget::measureFileNameWidth(const QString &fileName) {
  return fontMetrics.horizontalAdvance(fileName);
}

double FileExplorerWidget::measureContentWidth() {
  auto snapshot = fileExplorerController->getTreeSnapshot();
  double finalWidth = 0;

  // TODO(scarlet): Make this faster if possible?
  for (auto &node : snapshot.nodes) {
    QString lineText = QString::fromUtf8(node.name.c_str());
    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::updateDimensions() {
  auto nodeCount = fileExplorerController->getNodeCount();

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

// TODO(scarlet): Change this to use getCurrentContext().
void FileExplorerWidget::contextMenuEvent(QContextMenuEvent *event) {
  auto pasteInfo = fileExplorerController->getCurrentContext().paste_info;
  auto snapshot = fileExplorerController->getTreeSnapshot();
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
                                   .item_is_expanded = itemIsExpanded,
                                   .paste_info = pasteInfo};

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

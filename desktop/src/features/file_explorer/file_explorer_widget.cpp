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

  dragHoverTimer.setSingleShot(true);
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

  connect(&dragHoverTimer, &QTimer::timeout, this, [this] {
    if (dragHoveredNodePath != nullptr) {
      fileExplorerController->setExpanded(dragHoveredNodePath);
      redraw();
    }
  });

  // FileExplorerController -> FileExplorerWidget
  connect(fileExplorerController, &FileExplorerController::commandRequested,
          this, &FileExplorerWidget::commandRequested);
  connect(fileExplorerController, &FileExplorerController::requestFocusEditor,
          this, &FileExplorerWidget::requestFocusEditor);
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

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  ChangeSet changeSet =
      fileExplorerController->handleKeyPress(event->key(), event->modifiers());

  if (changeSet.redraw) {
    redraw();
  }

  if (changeSet.scroll) {
    auto node = fileExplorerController->getNode(
        [](const neko::FileNodeSnapshot &node) { return node.is_current; });

    if (node.foundNode()) {
      scrollToNode(node.index);
    }
  }

  switch (changeSet.fontSizeAdjustment) {
  case FontSizeAdjustment::Increase:
    increaseFontSize();
    break;
  case FontSizeAdjustment::Decrease:
    decreaseFontSize();
    break;
  case FontSizeAdjustment::Reset:
    resetFontSize();
    break;
  case FontSizeAdjustment::NoChange:
    break;
  }
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  const int row = convertMousePositionToRow(event->pos().y());
  bool wasLeftMouseButton = event->button() == Qt::LeftButton;

  auto node = fileExplorerController->handleNodeClick(row, wasLeftMouseButton);

  if (node.foundNode()) {
    draggedNode = node.nodeSnapshot;
    draggedNodeRow = row;
  } else {
    draggedNodeRow = -1;
  }

  if (wasLeftMouseButton) {
    dragStartPosition = event->pos();
    event->accept();
  }
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
  if (((event->buttons() & Qt::LeftButton) != 0U) && !isDragging &&
      draggedNodeRow != -1) {
    if ((event->pos() - dragStartPosition).manhattanLength() >=
        QApplication::startDragDistance()) {

      isDragging = true;

      performDrag(draggedNodeRow, draggedNode);

      isDragging = false;
      hoveredNodePath = nullptr;
      draggedNodeRow = 0;
      redraw();
    }
  }

  event->accept();
  redraw();
}

void FileExplorerWidget::mouseReleaseEvent(QMouseEvent *event) {
  const int row = convertMousePositionToRow(event->pos().y());
  bool wasLeftMouseButton = event->button() == Qt::LeftButton;

  fileExplorerController->handleNodeClickRelease(row, wasLeftMouseButton);

  if (!wasLeftMouseButton) {
    return;
  }

  event->accept();
  redraw();
}

void FileExplorerWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  const int row = convertMousePositionToRow(event->pos().y());
  bool wasLeftMouseButton = event->button() == Qt::LeftButton;

  fileExplorerController->handleNodeDoubleClick(row, wasLeftMouseButton);

  // If the click was not the left mouse button, don't do anything.
  if (!wasLeftMouseButton) {
    return;
  }

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
  double nodeNameWidth = measureFileNameWidth(QString::fromUtf8(node.name));
  int iconSize = static_cast<int>(lineHeight - ICON_ADJUSTMENT);
  double width =
      EDGE_INSET + iconSize + ICON_SPACING + nodeNameWidth + EDGE_INSET;
  const double height = lineHeight * 1.5;

  QPixmap dragPixmap(static_cast<int>(width), static_cast<int>(height));
  dragPixmap.fill(Qt::transparent);

  {
    auto ctx = getViewportContext();
    auto state = getRenderState();
    const double opacity = 0.85;

    QPainter painter(&dragPixmap);
    painter.setOpacity(opacity);
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
    int row = convertMousePositionToRow(event->position().y());
    const auto targetNode = fileExplorerController->getNodeByIndex(row);

    if (targetNode.foundNode()) {
      dragHoveredNodePath = QString::fromUtf8(targetNode.nodeSnapshot.path);
      dragHoverTimer.start(dragHoverMs);
    } else {
      dragHoveredNodePath = nullptr;
      dragHoverTimer.stop();
    }

    redraw();
    event->acceptProposedAction();
  }
}

void FileExplorerWidget::dropEvent(QDropEvent *event) {
  dragHoverTimer.stop();
  dragHoveredNodePath = nullptr;

  const int row = convertMousePositionToRow(event->position().y());
  const QByteArray encodedData =
      event->mimeData()->data("application/x-neko-file-explorer-node");

  fileExplorerController->handleNodeDrop(row, encodedData);
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
      .dragHoveredNodePath = dragHoveredNodePath,
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

void FileExplorerWidget::contextMenuEvent(QContextMenuEvent *event) {
  auto ctx = fileExplorerController->getCurrentContext();
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

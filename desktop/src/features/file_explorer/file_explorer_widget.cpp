#include "file_explorer_widget.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/services/file_io_service.h"
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
    : QScrollArea(parent), fileTreeBridge(props.fileTreeBridge),
      font(props.font), theme(props.theme), fontMetrics(font),
      contextMenuRegistry(*props.contextMenuRegistry),
      commandRegistry(*props.commandRegistry),
      themeProvider(props.themeProvider) {
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

void FileExplorerWidget::initialize(const std::string &path) {
  fileTreeBridge->setRootDir(path);
  emit directorySelected(path);
  loadDirectory(path);
}

void FileExplorerWidget::loadSavedDirectory(const std::string &path) {
  QString savedDir = QString::fromUtf8(path);

  initialize(savedDir.toStdString());
  directorySelectionButton->hide();
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
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  int index = 0;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      break;
    }

    index++;
  }

  scrollToNode(index);
  redraw();
  setFocus();
}

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);

  if (ctrl) {
    bool shouldRedraw = false;

    switch (event->key()) {
    case Qt::Key_C:
      handleCopy();
      return;
    case Qt::Key_V:
      shouldRedraw = true;
      handlePaste();
      break;
    case Qt::Key_X:
      handleCut();
      return;
    case Qt::Key_D:
      shouldRedraw = true;
      handleDuplicate();
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

  switch (event->key()) {
  case Qt::Key_Up:
    selectPrevNode();
    shouldScroll = true;
    break;
  case Qt::Key_Down:
    selectNextNode();
    shouldScroll = true;
    break;
  case Qt::Key_Left:
    handleLeft();
    shouldScroll = true;
    break;
  case Qt::Key_Right:
    handleRight();
    shouldScroll = true;
    break;
  case Qt::Key_Space:
    toggleSelectNode();
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    handleEnter();
    break;
  case Qt::Key_Delete:
    if (shift) {
      handleDeleteNoConfirm();
    } else {
      handleDeleteConfirm();
    }
  default:
    break;
  }

  if (shouldScroll) {
    auto snapshot = fileTreeBridge->getTreeSnapshot();
    int index = 0;
    for (auto &node : snapshot.nodes) {
      if (node.is_current) {
        break;
      }

      index++;
    }

    scrollToNode(index);
  }

  redraw();
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  const bool isLeftClick = (event->button() == Qt::LeftButton);

  auto snapshot = fileTreeBridge->getTreeSnapshot();
  const int row = convertMousePositionToRow(event->pos().y());

  const bool refocusClick = focusReceivedFromMouse;
  focusReceivedFromMouse = false;

  const int nodeCount = static_cast<int>(snapshot.nodes.size());

  // Clicked below the last visible row.
  if (row < 0 || row >= nodeCount) {
    if (isLeftClick) {
      if (refocusClick) {
        setFocus();
      } else {
        fileTreeBridge->clearCurrent();
        redraw();
      }
    } else {
      // Non-left click outside items; just clear the selection.
      fileTreeBridge->clearCurrent();
      redraw();
    }
    return;
  }

  auto &currentNode = snapshot.nodes[row];
  fileTreeBridge->setCurrent(currentNode.path.c_str());

  // If it is a non-left click, just update the current node.
  if (!isLeftClick) {
    redraw();
    return;
  }

  // If it is a left click, open the target file or toggle the target directory.
  if (currentNode.is_dir) {
    fileTreeBridge->toggleExpanded(currentNode.path.c_str());
    updateDimensions();
  } else {
    const auto fileStr = QString::fromUtf8(currentNode.path.c_str());
    emit fileSelected(fileStr.toStdString(), false);
  }

  redraw();
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
  auto snapshot = fileTreeBridge->getTreeSnapshot();

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

void FileExplorerWidget::applySelectedDirectory(const std::string &path) {
  if (path.empty()) {
    return;
  }

  initialize(path);
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

void FileExplorerWidget::loadDirectory(const std::string &path) {
  fileTreeBridge->setExpanded(path);
  updateDimensions();
  redraw();
}

double FileExplorerWidget::measureContent() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  double finalWidth = 0;

  // TODO(scarlet): Make this faster
  for (auto &node : snapshot.nodes) {
    QString lineText = QString::fromUtf8(node.name.c_str());
    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::updateDimensions() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();

  const double lineHeight = fontMetrics.height();
  const double viewportHeight =
      std::max(0.0, (static_cast<int>(snapshot.nodes.size()) * lineHeight) -
                        viewport()->height() + VIEWPORT_PADDING);
  const double viewportWidth =
      std::max(0.0, measureContent() - viewport()->width() + VIEWPORT_PADDING);

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

void FileExplorerWidget::handleEnter() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  neko::FileNodeSnapshot currentNode;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNode = node;
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is a directory, toggle expansion
  bool isDir = currentNode.is_dir;

  if (!currentNodePath.empty()) {
    if (isDir) {
      if (!currentNodeExpanded) {
        expandNode();
      } else {
        collapseNode();
      }
    } else {
      // Otherwise, open the file in the editor
      emit fileSelected(currentNodePath);
    }
  } else {
    // If no node is selected (and the tree is not empty), select the first node
    if (!snapshot.nodes.empty()) {
      fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    } else {
      return;
    }
  }
}

void FileExplorerWidget::handleLeft() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is collapsed, go to parent and collapse
  if (!currentNodeExpanded) {
    auto parentPath = fileTreeBridge->getParentNodePath(currentNodePath);
    if (parentPath != snapshot.root.c_str()) {
      fileTreeBridge->setCurrent(parentPath);
      fileTreeBridge->setCollapsed(parentPath);
    }
  } else {
    // Otherwise, collapse focused node and stay put
    collapseNode();
  }
}

void FileExplorerWidget::handleRight() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  neko::FileNodeSnapshot currentNode;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNode = node;
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is collapsed, expand and stay put
  if (!currentNodeExpanded) {
    expandNode();
  } else {
    // Otherwise, go to first child
    auto children = fileTreeBridge->getVisibleChildren(currentNodePath);

    if (!children.empty()) {
      fileTreeBridge->setCurrent(children[0].path.c_str());
    }
  }
}

void FileExplorerWidget::handleCut() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  QString currentNodePath;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = QString::fromUtf8(node.path);
    }
  }

  FileIoService::cut(currentNodePath);
}

void FileExplorerWidget::handleCopy() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  QString currentNodePath;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = QString::fromUtf8(node.path);
    }
  }

  FileIoService::copy(currentNodePath);
}

void FileExplorerWidget::handlePaste() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  QString currentNodePath;
  bool currentNodeIsDirectory = false;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = QString::fromUtf8(node.path);
      currentNodeIsDirectory = node.is_dir;
    }
  }

  auto parentNodePath = QString::fromUtf8(
      fileTreeBridge->getParentNodePath(currentNodePath.toStdString()));

  QString targetDirectory =
      currentNodeIsDirectory ? currentNodePath : parentNodePath;

  FileIoService::paste(targetDirectory);
  fileTreeBridge->refreshDirectory(currentNodeIsDirectory
                                       ? currentNodePath.toStdString()
                                       : parentNodePath.toStdString());
}

void FileExplorerWidget::handleDeleteConfirm() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  neko::FileNodeSnapshot currentNode;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNode = node;
      break;
    }
  }

  {
    using ItemType = DialogService::DeleteItemType;
    using Decision = DialogService::DeleteDecision;

    ItemType itemType =
        currentNode.is_dir ? ItemType::Directory : ItemType::File;

    const auto decision = DialogService::openDeleteConfirmationDialog(
        currentNode.name.c_str(), itemType, parentWidget());

    if (decision == Decision::Delete) {
      deleteItem(currentNode.path.c_str(), currentNode);
    }
  }
}

void FileExplorerWidget::handleDeleteNoConfirm() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  neko::FileNodeSnapshot currentNode;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNode = node;
      break;
    }
  }

  deleteItem(currentNode.path.c_str(), currentNode);
}

void FileExplorerWidget::deleteItem(const std::string &path,
                                    const neko::FileNodeSnapshot &currentNode) {
  auto prevNode = fileTreeBridge->getPreviousNode(path);
  auto parentPath = fileTreeBridge->getParentNodePath(path);
  bool currentIsDir = currentNode.is_dir;
  bool wasSuccessful = FileIoService::deleteItem(path.c_str());

  if (wasSuccessful) {
    fileTreeBridge->refreshDirectory(parentPath);
    fileTreeBridge->setExpanded(parentPath);
    fileTreeBridge->setCurrent(prevNode.path.c_str());
  }
}

void FileExplorerWidget::handleDuplicate() {
  // TODO(scarlet): Get the current node path from the tree/bridge directly?
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  QString currentNodePath;
  bool currentNodeIsDirectory = false;

  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = QString::fromUtf8(node.path);
      currentNodeIsDirectory = node.is_dir;
    }
  }

  auto parentNodePath = QString::fromUtf8(
      fileTreeBridge->getParentNodePath(currentNodePath.toStdString()));

  const auto duplicateResult = FileIoService::duplicate(currentNodePath);

  if (duplicateResult.success) {
    fileTreeBridge->refreshDirectory(parentNodePath.toStdString());
    fileTreeBridge->setCurrent(duplicateResult.newPath.toStdString());
  }
}

void FileExplorerWidget::selectNextNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (!snapshot.nodes.empty()) {
      fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    } else {
      return;
    }
    return;
  }

  // If we are at the bottom
  if (currentNodePath == snapshot.nodes.back().path.c_str()) {
    // Wrap to start
    fileTreeBridge->setCurrent(snapshot.nodes.begin()->path.c_str());
    return;
  }

  auto next = fileTreeBridge->getNextNode(currentNodePath);
  fileTreeBridge->setCurrent(next.path.c_str());
}

void FileExplorerWidget::selectPrevNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  // If we are at the top (root node)
  if (currentNodePath == snapshot.nodes[0].path.c_str()) {
    // Wrap to end
    fileTreeBridge->setCurrent(
        snapshot.nodes.at(snapshot.nodes.size() - 1).path.c_str());
    return;
  }

  auto prev = fileTreeBridge->getPreviousNode(currentNodePath);
  fileTreeBridge->setCurrent(prev.path.c_str());
}

void FileExplorerWidget::toggleSelectNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeBridge->toggleSelect(currentNodePath);
}

void FileExplorerWidget::expandNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeBridge->setExpanded(currentNodePath);
  updateDimensions();
}

void FileExplorerWidget::collapseNode() {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeBridge->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeBridge->setCollapsed(currentNodePath);
  updateDimensions();
}

int FileExplorerWidget::convertMousePositionToRow(double yPos) {
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();

  const int targetRow = static_cast<int>((yPos + scrollY) / lineHeight);

  return targetRow;
}

void FileExplorerWidget::contextMenuEvent(QContextMenuEvent *event) {
  auto snapshot = fileTreeBridge->getTreeSnapshot();
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

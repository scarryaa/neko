#include "file_explorer_widget.h"

FileExplorerWidget::FileExplorerWidget(neko::FileTree *tree,
                                       neko::ConfigManager &configManager,
                                       neko::ThemeManager &themeManager,
                                       QWidget *parent)
    : QScrollArea(parent), tree(tree), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::FileExplorer)),
      fontMetrics(font) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  setStyleSheet(UiUtils::getScrollBarStylesheet("FileExplorerWidget", "black"));

  directorySelectionButton = new QPushButton("Select a directory");
  directorySelectionButton->setStyleSheet(
      "QPushButton { background-color: #202020; color: #e0e0e0; border-radius: "
      "6px; padding: 8px 16px; font-size: 13px; }"
      "QPushButton:hover { background-color: #444444; border-color: #555555; }"
      "QPushButton:pressed { background-color: #222222; border-color: #333333; "
      "}");

  auto layout = new QVBoxLayout();
  layout->addWidget(directorySelectionButton, 0, Qt::AlignCenter);
  setLayout(layout);

  connect(directorySelectionButton, &QPushButton::clicked, this,
          [this]() { directorySelectionRequested(); });
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
}

FileExplorerWidget::~FileExplorerWidget() {}

void FileExplorerWidget::resizeEvent(QResizeEvent *event) {
  handleViewportUpdate();
}

void FileExplorerWidget::loadSavedDir() {
  auto rawPath = configManager.get_file_explorer_directory();
  QString savedDir = QString::fromUtf8(rawPath);

  if (!savedDir.isEmpty()) {
    initialize(savedDir.toStdString());
    rootPath = savedDir.toStdString();
    directorySelectionButton->hide();
  }
}

void FileExplorerWidget::directorySelectionRequested() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select a directory", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!dir.isEmpty()) {
    initialize(dir.toStdString());
    rootPath = dir.toStdString();
    directorySelectionButton->hide();
    configManager.set_file_explorer_directory(dir.toStdString());
  }
}

void FileExplorerWidget::initialize(std::string path) {
  tree->set_root_dir(path);
  emit directorySelected(path);
  loadDirectory(path);
  handleViewportUpdate();
}

void FileExplorerWidget::loadDirectory(const std::string path) {
  fileNodes = tree->get_children(path);
  fileCount = fileNodes.size();
  viewport()->repaint();
}

double FileExplorerWidget::measureContent() {
  double finalWidth = 0;

  // TODO: Make this faster
  for (int i = 0; i < fileCount; i++) {
    auto rawLine = fileNodes[i].name;
    QString lineText = QString::fromUtf8(rawLine);

    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::handleViewportUpdate() {
  auto viewportHeight = (fileCount * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureContent();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);
}

void FileExplorerWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  double horizontallDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().x() / 4.0;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(newHorizontalScrollOffset);
  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->repaint();
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  int row = convertMousePositionToRow(event->pos().y());

  if (row >= fileCount) {
    tree->clear_current();
    viewport()->repaint();
    return;
  }

  auto node = fileNodes[row];
  tree->set_current(node.path);

  if (node.is_dir) {
    tree->toggle_expanded(node.path);
    fileNodes = tree->get_visible_nodes();
    fileCount = fileNodes.size();
    handleViewportUpdate();
  } else {
    QString fileStr = QString::fromUtf8(node.path);
    emit fileSelected(fileStr.toStdString(), false);
  }

  viewport()->repaint();
}

int FileExplorerWidget::convertMousePositionToRow(double y) {
  const double lineHeight = fontMetrics.height();
  const int scrollX = horizontalScrollBar()->value();
  const int scrollY = verticalScrollBar()->value();

  const size_t targetRow = (y + scrollY) / lineHeight;

  return targetRow;
}

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;
  bool shouldUpdateViewport = false;

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
  }

  if (shouldUpdateViewport) {
    handleViewportUpdate();
  }

  if (shouldScroll) {
    int index = tree->get_current_index();
    scrollToNode(index);
  }

  viewport()->repaint();
}

void FileExplorerWidget::handleLeft() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is collapsed, go to parent and collapse
  if (!tree->is_expanded(rawCurrentPath)) {
    auto parent = tree->get_path_of_parent(rawCurrentPath);
    if (parent != rootPath) {
      tree->set_current(parent);
      tree->set_collapsed(parent);
    }
  } else {
    // Otherwise, collapse focused node and stay put
    collapseNode();
  }

  fileNodes = tree->get_visible_nodes();
  fileCount = fileNodes.size();
  handleViewportUpdate();
}

void FileExplorerWidget::handleRight() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is collapsed, expand and stay put
  if (!tree->is_expanded(rawCurrentPath)) {
    expandNode();
  } else {
    // Otherwise, go to first child
    auto children = tree->get_children(rawCurrentPath);

    if (!children.empty()) {
      tree->set_current(children[0].path);
    }
  }

  fileNodes = tree->get_visible_nodes();
  fileCount = fileNodes.size();
  handleViewportUpdate();
}

void FileExplorerWidget::handleEnter() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is a directory, toggle expansion
  auto currentFile = tree->get_node(rawCurrentPath);
  bool isDir = currentFile.is_dir;

  if (!rawCurrentPath.empty()) {
    if (isDir) {
      if (!tree->is_expanded(rawCurrentPath)) {
        expandNode();
      } else {
        collapseNode();
      }
    } else {
      // Otherwise, open the file in the editor
      emit fileSelected(rawCurrentPath.c_str());
    }
  } else {
    if (!fileNodes.empty()) {
      tree->set_current(fileNodes[0].path);
    } else {
      return;
    }
  }

  fileNodes = tree->get_visible_nodes();
  fileCount = fileNodes.size();
}

void FileExplorerWidget::selectNextNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (!fileNodes.empty()) {
      tree->set_current(fileNodes[0].path);
    } else {
      return;
    }
    return;
  }

  if (tree->get_current_index() == fileNodes.size() - 1) {
    // Wrap to start
    tree->set_current(fileNodes.begin()->path);
    return;
  }

  auto next = tree->get_next_node(rawCurrentPath);
  tree->set_current(next.path);
}

void FileExplorerWidget::selectPrevNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  if (tree->get_current_index() == 0) {
    // Wrap to end
    tree->set_current(fileNodes.at(fileNodes.size() - 1).path);
    return;
  }

  auto prev = tree->get_prev_node(rawCurrentPath);
  tree->set_current(prev.path);
}

void FileExplorerWidget::toggleSelectNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->toggle_select(rawCurrentPath);
}

void FileExplorerWidget::expandNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->set_expanded(rawCurrentPath);
  fileNodes = tree->get_visible_nodes();
  fileCount = fileNodes.size();

  handleViewportUpdate();
}

void FileExplorerWidget::collapseNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->set_collapsed(rawCurrentPath);
  fileNodes = tree->get_visible_nodes();
  fileCount = fileNodes.size();

  handleViewportUpdate();
}

void FileExplorerWidget::scrollToNode(int index) {
  const double viewportHeight = viewport()->height();
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();
  const size_t nodeY = (index * lineHeight);

  if (nodeY + lineHeight > viewportHeight - VIEWPORT_PADDING + scrollY) {
    verticalScrollBar()->setValue(nodeY - viewportHeight + VIEWPORT_PADDING +
                                  lineHeight);
  } else if (nodeY < scrollY + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(nodeY - VIEWPORT_PADDING);
  }
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  drawFiles(&painter, fileCount, fileNodes);
}

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   rust::Vec<neko::FileNode> nodes) {
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();

  int startRow = std::floor(verticalOffset / lineHeight);
  int endRow = std::ceil((verticalOffset + viewportHeight) / lineHeight);

  startRow = std::max(0, startRow);
  endRow = std::min((int)count, endRow);

  for (int i = startRow; i < endRow; i++) {
    double y = (i * lineHeight) - verticalOffset;
    drawFile(painter, -horizontalOffset, y, nodes[i]);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double x, double y,
                                  neko::FileNode node) {
  painter->setFont(font);

  double indent = node.depth * 20.0;
  double viewportWidth = viewport()->width();
  // Adjusted to avoid overlapping with the splitter
  double viewportWidthAdjusted = viewportWidth - 1.0;
  double lineHeight = fontMetrics.height();
  double horizontalOffset = horizontalScrollBar()->value();

  bool isSelected = tree->is_selected(node.path);
  bool isCurrent = tree->is_current(node.path);

  // Selection background
  if (isSelected) {
    painter->setBrush(SELECTION_COLOR);
    painter->setPen(Qt::NoPen);
    painter->drawRect(
        QRectF(x, y, viewportWidth + horizontalOffset, lineHeight));
  }

  // Current item border
  if (isCurrent && hasFocus()) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(SELECTION_PEN);
    painter->drawRect(
        QRectF(x, y, viewportWidthAdjusted + horizontalOffset, lineHeight));
  }

  // Get appropriate icon
  QIcon icon;
  if (node.is_dir) {
    bool isExpanded = tree->is_expanded(node.path);

    icon = QApplication::style()->standardIcon(
        isExpanded ? QStyle::SP_DirOpenIcon : QStyle::SP_DirIcon);
  } else {
    icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
  }

  int iconSize = lineHeight - 2;
  QPixmap pixmap = icon.pixmap(iconSize, iconSize);

  // Draw icon with indentation
  double iconX = x + indent + 2;
  double iconY = y + (lineHeight - iconSize) / 2;
  painter->drawPixmap(QPointF(iconX, iconY), pixmap);

  // Draw text after icon
  double textX = iconX + iconSize + 4;
  painter->setBrush(Qt::white);
  painter->setPen(Qt::white);
  painter->drawText(QPointF(textX, y + fontMetrics.ascent()),
                    QString::fromUtf8(node.name));
}

#include "file_explorer_widget.h"
#include "features/config/config_manager.h"
#include <neko_core.h>

FileExplorerWidget::FileExplorerWidget(FileTree *tree, QWidget *parent)
    : QScrollArea(parent), tree(tree),
      font(new QFont(
          "IBM Plex Sans",
          ConfigManager::getInstance().getConfig().fileExplorerFontSize)),
      fontMetrics(QFontMetricsF(*font)) {
  setFocusPolicy(Qt::StrongFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCornerWidget(nullptr);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  setStyleSheet(
      "QAbstractScrollArea::corner {"
      "  background: transparent;"
      "}"
      "QScrollBar:vertical {"
      "  background: transparent;"
      "  width: 12px;"
      "  margin: 0px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #555555;"
      "  min-height: 20px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: #666666;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: none;"
      "}"
      "QScrollBar:horizontal {"
      "  background: transparent;"
      "  height: 12px;"
      "  margin: 0px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: #555555;"
      "  min-width: 20px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "  background: #666666;"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "}"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: none;"
      "}"
      "FileExplorerWidget {"
      "  background: black;"
      "}");

  directorySelectionButton = new QPushButton("Select a directory");
  directorySelectionButton->setStyleSheet("QPushButton {"
                                          "  background-color: #202020;"
                                          "  color: #e0e0e0;"
                                          "  border-radius: 6px;"
                                          "  padding: 8px 16px;"
                                          "  font-size: 13px;"
                                          "}"
                                          "QPushButton:hover {"
                                          "  background-color: #444444;"
                                          "  border-color: #555555;"
                                          "}"
                                          "QPushButton:pressed {"
                                          "  background-color: #222222;"
                                          "  border-color: #333333;"
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
  QString savedDir =
      ConfigManager::getInstance().getConfig().fileExplorerDirectory;
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
    ConfigManager::getInstance().getConfig().fileExplorerDirectory = dir;
  }
}

void FileExplorerWidget::initialize(std::string path) {
  neko_file_tree_set_root_path(tree, path.c_str());
  emit directorySelected(path);
  loadDirectory(path);
  handleViewportUpdate();
}

void FileExplorerWidget::loadDirectory(const std::string path) {
  neko_file_tree_get_children(tree, path.c_str(), &fileNodes, &fileCount);
  viewport()->repaint();
}

double FileExplorerWidget::measureContent() {
  double finalWidth = 0;

  // TODO: Make this faster
  for (int i = 0; i < fileCount; i++) {
    const char *line = fileNodes[i].name;
    QString lineText = QString::fromStdString(line);

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
  if (tree == nullptr)
    return;

  int row = convertMousePositionToRow(event->pos().y());
  if (row >= fileCount) {
    neko_file_tree_clear_current(tree);
    viewport()->repaint();
    return;
  }

  auto node = fileNodes[row];
  neko_file_tree_set_current(tree, node.path);

  if (node.is_dir) {
    neko_file_tree_toggle_expanded(tree, node.path);
    neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);
    handleViewportUpdate();
  } else {
    emit fileSelected(node.path, false);
  }

  viewport()->repaint();
}

int FileExplorerWidget::convertMousePositionToRow(double y) {
  if (tree == nullptr)
    return 0;

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
    int index = neko_file_tree_get_index(tree);
    scrollToNode(index);
  }

  viewport()->repaint();
}

void FileExplorerWidget::handleLeft() {
  auto currentPath = neko_file_tree_get_current(tree);

  // If focused node is collapsed, go to parent and collapse
  if (!neko_file_tree_is_expanded(tree, currentPath)) {
    auto parent = neko_file_tree_get_parent(tree, currentPath);
    if (parent != nullptr && parent != rootPath) {
      neko_file_tree_set_current(tree, parent);
      neko_file_tree_set_collapsed(tree, parent);
      neko_string_free(parent);
    }
  } else {
    // Otherwise, collapse focused node and stay put
    collapseNode();
  }

  if (currentPath != nullptr) {
    neko_string_free(const_cast<char *>(currentPath));
  }

  neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);
  handleViewportUpdate();
}

void FileExplorerWidget::handleRight() {
  auto currentPath = neko_file_tree_get_current(tree);

  // If focused node is collapsed, expand and stay put
  if (!neko_file_tree_is_expanded(tree, currentPath)) {
    expandNode();
  } else {
    // Otherwise, go to first child
    const FileNode *children;
    size_t childCount = 0;

    neko_file_tree_get_children(tree, currentPath, &children, &childCount);

    if (childCount > 0) {
      neko_file_tree_set_current(tree, children[0].path);
    }
  }

  if (currentPath != nullptr) {
    neko_string_free(const_cast<char *>(currentPath));
  }

  neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);
  handleViewportUpdate();
}

void FileExplorerWidget::handleEnter() {
  auto currentPath = neko_file_tree_get_current(tree);

  // If focused node exists
  if (currentPath != nullptr) {
    // If focused node is a directory, toggle expansion
    const FileNode *currentFile = neko_file_tree_get_node(tree, currentPath);
    bool isDir = currentFile->is_dir;

    if (isDir) {
      if (!neko_file_tree_is_expanded(tree, currentPath)) {
        expandNode();
      } else {
        collapseNode();
      }
    } else {
      // Otherwise, open the file in the editor
      if (currentFile != nullptr) {
        emit fileSelected(currentPath);
      }
    }
  } else {
    neko_file_tree_set_current(tree, fileNodes[0].path);
  }

  if (currentPath != nullptr) {
    neko_string_free(const_cast<char *>(currentPath));
  }

  neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);
}

void FileExplorerWidget::selectNextNode() {
  auto currentPath = neko_file_tree_get_current(tree);

  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  auto next = neko_file_tree_next(tree, currentPath);
  if (next != nullptr) {
    neko_file_tree_set_current(tree, next->path);

    neko_string_free(const_cast<char *>(currentPath));
  }
}

void FileExplorerWidget::selectPrevNode() {
  auto currentPath = neko_file_tree_get_current(tree);

  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  auto prev = neko_file_tree_prev(tree, currentPath);
  if (prev != nullptr) {
    neko_file_tree_set_current(tree, prev->path);

    neko_string_free(const_cast<char *>(currentPath));
  }
}

void FileExplorerWidget::toggleSelectNode() {
  auto currentPath = neko_file_tree_get_current(tree);

  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  neko_file_tree_toggle_select(tree, currentPath);

  neko_string_free(const_cast<char *>(currentPath));
}

void FileExplorerWidget::expandNode() {
  auto currentPath = neko_file_tree_get_current(tree);
  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  neko_file_tree_set_expanded(tree, currentPath);
  neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);

  neko_string_free(const_cast<char *>(currentPath));
  handleViewportUpdate();
}

void FileExplorerWidget::collapseNode() {
  auto currentPath = neko_file_tree_get_current(tree);
  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  neko_file_tree_set_collapsed(tree, currentPath);
  neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);

  neko_string_free(const_cast<char *>(currentPath));
  handleViewportUpdate();
}

void FileExplorerWidget::scrollToNode(int index) {
  if (tree == nullptr)
    return;

  const double viewportHeight = viewport()->height();
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();
  const size_t nodeY = (index * lineHeight);

  if (nodeY + lineHeight > viewportHeight + scrollY) {
    verticalScrollBar()->setValue(nodeY - viewportHeight + lineHeight);
  } else if (nodeY < scrollY) {
    verticalScrollBar()->setValue(nodeY);
  }
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  drawFiles(&painter, fileCount, fileNodes);
}

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   const FileNode *nodes) {
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();

  for (size_t i = 0; i < count; i++) {
    double y = (i * lineHeight) - verticalOffset;

    drawFile(painter, -horizontalOffset, y, &nodes[i]);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double x, double y,
                                  const FileNode *node) {
  painter->setFont(*font);

  double indent = node->depth * 20.0;
  double viewportWidth = viewport()->width();
  // Adjusted to avoid overlapping with the splitter
  double viewportWidthAdjusted = viewportWidth - 1.0;
  double lineHeight = fontMetrics.height();
  double horizontalOffset = horizontalScrollBar()->value();

  bool isSelected = neko_file_tree_is_selected(tree, node->path);
  bool isCurrent = neko_file_tree_is_current(tree, node->path);

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
  if (node->is_dir) {
    bool isExpanded = neko_file_tree_is_expanded(tree, node->path);

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
                    QString::fromUtf8(node->name));
}

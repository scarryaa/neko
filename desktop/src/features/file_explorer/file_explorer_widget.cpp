#include "file_explorer_widget.h"
#include <neko_core.h>

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QScrollArea(parent), tree(nullptr),
      font(new QFont("IBM Plex Sans", 15.0)),
      fontMetrics(QFontMetricsF(*font)) {
  setFocusPolicy(Qt::StrongFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCornerWidget(nullptr);

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
      "}");

  directorySelectionButton = new QPushButton("Select a directory");

  auto layout = new QVBoxLayout();
  layout->addWidget(directorySelectionButton);

  setLayout(layout);

  connect(directorySelectionButton, &QPushButton::clicked, this, [this]() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select a directory", QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
      initialize(dir.toStdString());
      rootPath = dir.toStdString();
      directorySelectionButton->hide();
    }
  });
}

FileExplorerWidget::~FileExplorerWidget() { neko_file_tree_free(tree); }

void FileExplorerWidget::initialize(std::string path) {
  tree = neko_file_tree_new(path.c_str());
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
  double verticalDelta = event->angleDelta().y() / 8.0;
  double horizontallDelta = event->angleDelta().x() / 8.0;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(newHorizontalScrollOffset);
  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->repaint();
}

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;
  bool shouldUpdateViewport = false;

  switch (event->key()) {
  case Qt::Key_Up:
    selectPrevNode();
    break;
  case Qt::Key_Down: {
    selectNextNode();
    break;
  }
  case Qt::Key_Left:
    break;
  case Qt::Key_Right:
    toggleExpandNode();
    neko_file_tree_get_visible_nodes(tree, &fileNodes, &fileCount);

    shouldUpdateViewport = true;
    break;
  case Qt::Key_Space:
    toggleSelectNode();
    break;
  }

  if (shouldUpdateViewport) {
    handleViewportUpdate();
  }

  // if (shouldScroll) {
  //   scrollToCursor();
  // }

  viewport()->repaint();
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

void FileExplorerWidget::toggleExpandNode() {
  auto currentPath = neko_file_tree_get_current(tree);

  if (currentPath == nullptr) {
    if (fileNodes == nullptr) {
      return;
    }

    neko_file_tree_set_current(tree, fileNodes[0].path);
    return;
  }

  neko_file_tree_toggle_expanded(tree, currentPath);
  loadDirectory(rootPath);

  neko_string_free(const_cast<char *>(currentPath));
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {}

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

  bool isSelected = neko_file_tree_is_selected(tree, node->path);
  bool isCurrent = neko_file_tree_is_current(tree, node->path);

  // Selection background
  if (isSelected) {
    painter->setBrush(SELECTION_COLOR);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRectF(x, y, viewportWidth, lineHeight));
  }

  // Current item border
  if (isCurrent) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(SELECTION_PEN);
    painter->drawRect(QRectF(x, y, viewportWidthAdjusted, lineHeight));
  }

  // Get appropriate icon
  QIcon icon;
  if (node->is_dir) {
    icon = QIcon::fromTheme(
        "folder", QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  } else {
    icon = QIcon::fromTheme(
        "text-x-generic",
        QApplication::style()->standardIcon(QStyle::SP_FileIcon));
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

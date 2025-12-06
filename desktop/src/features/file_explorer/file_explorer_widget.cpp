#include "file_explorer_widget.h"

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

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  drawFiles(&painter, fileCount, fileNodes);
}

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   const FileNode *nodes) {
  painter->setBrush(Qt::white);
  painter->setPen(Qt::white);

  double lineHeight = fontMetrics.height();

  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();

  for (size_t i = 0; i < count; i++) {
    auto actualY =
        (i * fontMetrics.height()) +
        (fontMetrics.height() + fontMetrics.ascent() - fontMetrics.descent()) /
            2.0 -
        verticalOffset;

    drawFile(painter, -horizontalOffset, actualY, nodes[i].name);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double x, double y,
                                  std::string fileName) {
  painter->drawText(QPointF(x, y), QString::fromStdString(fileName));
}

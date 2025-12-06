#include "file_explorer_widget.h"

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QScrollArea(parent), tree(nullptr),
      font(new QFont("IBM Plex Sans", 15.0)),
      fontMetrics(QFontMetricsF(*font)) {
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
}

void FileExplorerWidget::loadDirectory(const std::string path) {
  neko_file_tree_get_children(tree, path.c_str(), &fileNodes, &fileCount);

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

#include "file_explorer_widget.h"

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QScrollArea(parent), tree(nullptr) {
  tree = neko_file_tree_new("");
  loadDirectory("");
}

FileExplorerWidget::~FileExplorerWidget() { neko_file_tree_free(tree); }

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

  for (size_t i = 0; i < count; i++) {
    drawFile(painter, 20, 20, nodes[i].name);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double x, double y,
                                  std::string fileName) {
  painter->drawText(QPointF(x, y), QString::fromStdString(fileName));
}

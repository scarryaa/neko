#ifndef FILEEXPLORERWIDGET_H
#define FILEEXPLORERWIDGET_H

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontMetricsF>
#include <QIcon>
#include <QPainter>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <neko_core.h>

struct FileTree;

class FileExplorerWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit FileExplorerWidget(QWidget *parent = nullptr);
  ~FileExplorerWidget();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void drawFiles(QPainter *painter, size_t count, const FileNode *nodes);
  void drawFile(QPainter *painter, double x, double y, const FileNode *node);

  void loadDirectory(const std::string path);
  void initialize(std::string path);

  double measureContent();
  void handleViewportUpdate();

  void selectNextNode();
  void selectPrevNode();
  void toggleSelectNode();

  FileTree *tree;
  size_t fileCount = 0;
  const FileNode *fileNodes = nullptr;
  QPushButton *directorySelectionButton;
  QFont *font;
  QFontMetricsF fontMetrics;

  double VIEWPORT_PADDING = 74.0;
  QColor SELECTION_COLOR = QColor(66, 181, 212, 50);
  QPen SELECTION_PEN = QPen(QColor(66, 181, 212, 175), 1);
};

#endif

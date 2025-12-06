#ifndef FILEEXPLORERWIDGET_H
#define FILEEXPLORERWIDGET_H

#include <QFileDialog>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
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
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void drawFiles(QPainter *painter, size_t count, const FileNode *nodes);
  void drawFile(QPainter *painter, double x, double y, std::string fileName);
  void loadDirectory(const std::string path);
  void initialize(std::string path);
  double measureContent();
  void handleViewportUpdate();

  FileTree *tree;
  size_t fileCount = 0;
  const FileNode *fileNodes = nullptr;
  QPushButton *directorySelectionButton;
  QFont *font;
  QFontMetricsF fontMetrics;

  double VIEWPORT_PADDING = 74.0;
};

#endif

#ifndef FILEEXPLORERWIDGET_H
#define FILEEXPLORERWIDGET_H

#include <QPainter>
#include <QPoint>
#include <QScrollArea>
#include <neko_core.h>

struct FileTree;

class FileExplorerWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit FileExplorerWidget(QWidget *parent = nullptr);
  ~FileExplorerWidget();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  void drawFiles(QPainter *painter, size_t count, const FileNode *nodes);
  void drawFile(QPainter *painter, double x, double y, std::string fileName);
  void loadDirectory(const std::string path);

  FileTree *tree;
  size_t fileCount = 0;
  const FileNode *fileNodes = nullptr;
};

#endif

#ifndef FILEEXPLORERWIDGET_H
#define FILEEXPLORERWIDGET_H

#include <QScrollArea>

struct FileTree;

class FileExplorerWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit FileExplorerWidget(QWidget *parent = nullptr);
  ~FileExplorerWidget();

protected:
private:
  FileTree *tree;
};

#endif

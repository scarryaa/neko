#ifndef FILE_EXPLORER_WIDGET_H
#define FILE_EXPLORER_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFont>
#include <QFontMetricsF>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <string>

struct FileTree;

class FileExplorerWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit FileExplorerWidget(neko::FileTree *tree,
                              neko::ConfigManager &configManager,
                              neko::ThemeManager &themeManager,
                              QWidget *parent = nullptr);
  ~FileExplorerWidget();

  void initialize(std::string path);
  void loadSavedDir();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

signals:
  void directorySelected(const std::string path);
  void fileSelected(const std::string path, bool shouldFocusEditor = true);

public slots:
  void directorySelectionRequested();

private:
  void drawFiles(QPainter *painter, size_t count, rust::Vec<neko::FileNode>);
  void drawFile(QPainter *painter, double x, double y, neko::FileNode node);

  void loadDirectory(const std::string path);

  double measureContent();
  void handleViewportUpdate();
  void scrollToNode(int index);

  void handleEnter();
  void handleLeft();
  void handleRight();
  void handleCopy();
  void handlePaste();
  bool copyRecursively(QString sourceFolder, QString destFolder);
  void handleDeleteConfirm();
  void handleDeleteNoConfirm();
  void deleteItem(std::string path, neko::FileNode currentNode);

  void selectNextNode();
  void selectPrevNode();
  void toggleSelectNode();
  void expandNode();
  void collapseNode();

  int convertMousePositionToRow(double y);

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  neko::FileTree *tree;
  size_t fileCount = 0;
  rust::Vec<neko::FileNode> fileNodes;
  std::string rootPath;
  QPushButton *directorySelectionButton;
  QFont font;
  QFontMetricsF fontMetrics;

  double VIEWPORT_PADDING = 74.0;
  QColor SELECTION_COLOR = QColor(66, 181, 212, 50);
  QPen SELECTION_PEN = QPen(QColor(66, 181, 212, 175), 1);
};

#endif // FILE_EXPLORER_WIDGET_H

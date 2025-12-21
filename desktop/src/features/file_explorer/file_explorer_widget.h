#ifndef FILE_EXPLORER_WIDGET_H
#define FILE_EXPLORER_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtDebug>

struct FileTree;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
class QWheelEvent;

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
  void applyTheme();
  void showItem(const QString &path);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

signals:
  void directorySelected(const std::string path);
  void fileSelected(const std::string path, bool shouldFocusEditor = true);

public slots:
  void directorySelectionRequested();

private:
  void redraw();
  void drawFiles(QPainter *painter, size_t count, rust::Vec<neko::FileNode>);
  void drawFile(QPainter *painter, double x, double y, neko::FileNode node);

  void loadDirectory(const std::string path);
  void setFileNodes(rust::Vec<neko::FileNode> nodes,
                    bool updateScrollbars = true);
  void refreshVisibleNodes(bool updateScrollbars = true);

  double measureContent();
  void updateDimensions();
  void handleViewportUpdate();
  void scrollToNode(int index);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(double newFontSize);

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
  bool focusReceivedFromMouse = false;

  double ICON_EDGE_PADDING = 10.0;
  double ICON_ADJUSTMENT = 6.0;
  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  double VIEWPORT_PADDING = 74.0;
};

#endif // FILE_EXPLORER_WIDGET_H

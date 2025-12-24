#ifndef FILE_EXPLORER_WIDGET_H
#define FILE_EXPLORER_WIDGET_H

#include "features/file_explorer/controllers/file_tree_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
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
  explicit FileExplorerWidget(FileTreeController *fileTreeController,
                              neko::ConfigManager &configManager,
                              neko::ThemeManager &themeManager,
                              QWidget *parent = nullptr);
  ~FileExplorerWidget() override = default;

  void initialize(const std::string &path);
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
  void directorySelected(std::string path);
  void fileSelected(std::string path, bool shouldFocusEditor = true);

public slots:
  void directorySelectionRequested();

private:
  void redraw();
  void drawFiles(QPainter *painter, size_t count,
                 rust::Vec<neko::FileNodeSnapshot>);
  void drawFile(QPainter *painter, double xPos, double yPos,
                const neko::FileNodeSnapshot &node);
  void connectSignals();

  void loadDirectory(const std::string &path);

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
  bool copyRecursively(const QString &sourceFolder, const QString &destFolder);
  void handleDeleteConfirm();
  void handleDeleteNoConfirm();
  void deleteItem(const std::string &path,
                  const neko::FileNodeSnapshot &currentNode);

  void selectNextNode();
  void selectPrevNode();
  void toggleSelectNode();
  void expandNode();
  void collapseNode();

  int convertMousePositionToRow(double yPos);

  FileTreeController *fileTreeController;
  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QPushButton *directorySelectionButton;
  QFont font;
  QFontMetricsF fontMetrics;
  bool focusReceivedFromMouse = false;

  static constexpr double ICON_EDGE_PADDING = 10.0;
  static constexpr double ICON_ADJUSTMENT = 6.0;
  static constexpr double FONT_STEP = 2.0;
  static constexpr double DEFAULT_FONT_SIZE = 15.0;
  static constexpr double FONT_UPPER_LIMIT = 96.0;
  static constexpr double FONT_LOWER_LIMIT = 6.0;
  static constexpr double VIEWPORT_PADDING = 74.0;
  static constexpr double NODE_INDENT = 20.0;
  static constexpr double SELECTION_ALPHA = 60.0;
  static constexpr double SCROLL_WHEEL_DIVIDER = 4.0;
};

#endif // FILE_EXPLORER_WIDGET_H

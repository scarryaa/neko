#ifndef FILE_EXPLORER_WIDGET_H
#define FILE_EXPLORER_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/file_explorer/controllers/file_explorer_controller.h"
#include "features/file_explorer/render/types/types.h"
#include "theme/theme_provider.h"
#include "theme/types/types.h"
#include "types/qt_types_fwd.h"
#include <QFont>
#include <QFontMetricsF>
#include <QPoint>
#include <QScrollArea>

QT_FWD(QFocusEvent, QKeyEvent, QMouseEvent, QPaintEvent, QPushButton,
       QResizeEvent, QWheelEvent, QPainter, QString);

class FileExplorerWidget : public QScrollArea {
  Q_OBJECT

public:
  struct FileExplorerProps {
    FileTreeBridge *fileTreeBridge;
    QFont font;
    FileExplorerTheme theme;
    ThemeProvider *themeProvider;
    ContextMenuRegistry *contextMenuRegistry;
    CommandRegistry *commandRegistry;
  };

  explicit FileExplorerWidget(const FileExplorerProps &props,
                              QWidget *parent = nullptr);
  ~FileExplorerWidget() override = default;

  void setAndApplyTheme(const FileExplorerTheme &newTheme);
  void redraw();
  void updateDimensions();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

signals:
  void fileSelected(QString path, bool shouldFocusEditor = true);
  void fontSizeChanged(double newSize);
  void directoryPersistRequested(const QString &path);
  void directorySelected(const QString &path);
  void directorySelectionRequested();
  void commandRequested(const std::string &commandId,
                        const neko::FileExplorerContextFfi &ctx,
                        bool bypassDeleteConfirmation);
  void requestFocusEditor(bool shouldFocus);

public slots:
  void itemRevealRequested();
  void applySelectedDirectory(const QString &path);

  void onRootDirectoryChanged();

private:
  void drawFiles(QPainter *painter, size_t count,
                 rust::Vec<neko::FileNodeSnapshot>);
  void drawFile(QPainter *painter, double xPos, double yPos,
                const neko::FileNodeSnapshot &node);
  void connectSignals();

  void loadDirectory(const std::string &path);

  double measureFileNameWidth(const QString &fileName);
  double measureContentWidth();
  void handleViewportUpdate();
  void scrollToNode(int index);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(double newFontSize);

  int convertMousePositionToRow(double yPos);

  void triggerCommand(const std::string &commandId,
                      bool bypassDeleteConfirmation = false, int index = -1,
                      const std::string &targetNodePath = "",
                      const std::string &destinationNodePath = "");

  void performDrag(int row, const neko::FileNodeSnapshot &node);

  FileExplorerRenderState getRenderState();
  FileExplorerViewportContext getViewportContext();

  FileExplorerController *fileExplorerController;

  QPushButton *directorySelectionButton;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  ThemeProvider *themeProvider;

  FileExplorerTheme theme;
  QFont font;
  QFontMetricsF fontMetrics;

  QString hoveredNodePath;
  neko::FileNodeSnapshot draggedNode;
  bool isDragging = false;
  QPoint dragStartPosition;

  static constexpr double FONT_STEP = 2.0;
  static constexpr double DEFAULT_FONT_SIZE = 15.0;
  static constexpr double FONT_UPPER_LIMIT = 96.0;
  static constexpr double FONT_LOWER_LIMIT = 6.0;
  static constexpr double VIEWPORT_PADDING = 74.0;
  static constexpr double SCROLL_WHEEL_DIVIDER = 4.0;
  static constexpr double ICON_EDGE_PADDING = 10.0;
  static constexpr double ICON_ADJUSTMENT = 6.0;
  static constexpr double EDGE_INSET = 12.0;
  static constexpr double ICON_SPACING = 4.0;

  static constexpr double GHOST_PADDING = 8.0;
  static constexpr double GHOST_BACKGROUND_ALPHA = 180.0;
};

#endif // FILE_EXPLORER_WIDGET_H

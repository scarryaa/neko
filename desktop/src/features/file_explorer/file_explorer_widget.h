#ifndef FILE_EXPLORER_WIDGET_H
#define FILE_EXPLORER_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/file_explorer/controllers/file_explorer_controller.h"
#include "theme/theme_provider.h"
#include "theme/types/types.h"
#include "types/qt_types_fwd.h"
#include <QFont>
#include <QFontMetricsF>
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
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

signals:
  void fileSelected(QString path, bool shouldFocusEditor = true);
  void fontSizeChanged(double newSize);
  void directoryPersistRequested(const QString &path);
  void directorySelected(const QString &path);
  void directorySelectionRequested();
  void commandRequested(const std::string &commandId,
                        const neko::FileExplorerContextFfi &ctx,
                        bool bypassDeleteConfirmation);

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

  double measureContentWidth();
  void handleViewportUpdate();
  void scrollToNode(int index);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(double newFontSize);

  int convertMousePositionToRow(double yPos);

  void triggerCommand(const std::string &commandId,
                      bool bypassDeleteConfirmation = false);

  FileExplorerController fileExplorerController;

  QPushButton *directorySelectionButton;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  ThemeProvider *themeProvider;

  FileExplorerTheme theme;
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

#ifndef STATUS_BAR_WIDGET_H
#define STATUS_BAR_WIDGET_H

#include "features/editor/controllers/editor_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "theme/theme_types.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>

class StatusBarWidget : public QWidget {
  Q_OBJECT

public:
  explicit StatusBarWidget(EditorController *editorController,
                           neko::ConfigManager &configManager,
                           const StatusBarTheme &theme,
                           QWidget *parent = nullptr);
  ~StatusBarWidget() override = default;

  void setAndApplyTheme(const StatusBarTheme &newTheme);
  void updateCursorPosition(int row, int col, int numberOfCursors);
  void showCursorPositionInfo();

signals:
  void fileExplorerToggled();
  void cursorPositionClicked();

public slots:
  void onCursorPositionChanged(int row, int col, int numberOfCursors);
  void onTabClosed(int numberOfTabs);
  void onFileExplorerToggledExternally(bool isOpen);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  void onFileExplorerToggled();
  void onCursorPositionClicked();

  EditorController *editorController;
  neko::ConfigManager &configManager;
  double m_height;
  QPushButton *fileExplorerToggleButton;
  QPushButton *cursorPosition;

  StatusBarTheme theme;

  static double constexpr ICON_SIZE = 18.0;
  static double constexpr HORIZONTAL_CONTENT_MARGIN = 10.0;
  static double constexpr VERTICAL_CONTENT_MARGIN = 5.0;
  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
};

#endif // STATUS_BAR_WIDGET_H

#ifndef STATUS_BAR_WIDGET_H
#define STATUS_BAR_WIDGET_H

class EditorController;
class ConfigManager;

#include "theme/theme_types.h"
#include "types/qt_types_fwd.h"
#include <QWidget>

QT_FWD(QPaintEvent, QPushButton);

class StatusBarWidget : public QWidget {
  Q_OBJECT

public:
  struct StatusBarProps {
    EditorController *editorController;
    QFont font;
    StatusBarTheme theme;
    bool fileExplorerInitiallyShown;
  };

  explicit StatusBarWidget(const StatusBarProps &props,
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
  double m_height;
  QPushButton *fileExplorerToggleButton;
  QPushButton *cursorPosition;
  QFont font;

  StatusBarTheme theme;

  static double constexpr ICON_SIZE = 18.0;
  static double constexpr HORIZONTAL_CONTENT_MARGIN = 10.0;
  static double constexpr VERTICAL_CONTENT_MARGIN = 5.0;
  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
};

#endif // STATUS_BAR_WIDGET_H

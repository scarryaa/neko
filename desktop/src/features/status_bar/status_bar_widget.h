#ifndef STATUS_BAR_WIDGET_H
#define STATUS_BAR_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
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
  explicit StatusBarWidget(neko::Editor *editor,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager,
                           QWidget *parent = nullptr);
  ~StatusBarWidget();

  void applyTheme();
  void setEditor(neko::Editor *newEditor);
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

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  neko::Editor *editor;
  double m_height;
  QPushButton *fileExplorerToggleButton;
  QPushButton *cursorPosition;
};

#endif // STATUS_BAR_WIDGET_H

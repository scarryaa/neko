#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/palette_divider.h"
#include "features/command_palette/palette_frame.h"
#include "utils/gui_utils.h"
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <neko-core/src/ffi/mod.rs.h>

class CommandPaletteWidget : public QWidget {
  Q_OBJECT

public:
  explicit CommandPaletteWidget(neko::ThemeManager &themeManager,
                                neko::ConfigManager &configManager,
                                QWidget *parent = nullptr);
  ~CommandPaletteWidget();

  void jumpToRowColumn(int currentRow = 0, int currentCol = 0, int maxCol = 1,
                       int lineCount = 1);

signals:
  void goToPositionRequested(int row, int col);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QWidget *parent;
  QWidget *shortcutsContainer;
  QToolButton *shortcutsToggle;
  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;

  const double TOP_OFFSET = 300.0;
  const double SHADOW_X_OFFSET = 0.0;
  const double SHADOW_Y_OFFSET = 5.0;
  const double SHADOW_BLUR_RADIUS = 25.0;
  const double CONTENT_MARGIN = 20.0; // Content margin for drop shadow
  const double WIDTH = 800.0;
  const double HEIGHT = 300.0;
  const int MIN_WIDTH = 360;

  PaletteFrame *mainFrame;
  QVBoxLayout *frameLayout;
  QLineEdit *jumpInput;
  int maxLineCount = 1;
  int maxColumn = 1;
  int maxRow = 1;
  int currentRow = 1;
  bool showJumpShortcuts = false;

  enum class Mode { None, GoToPosition };
  Mode currentMode = Mode::None;

  void clearContent();
  void buildJumpContent(int currentRow, int currentCol, int maxCol,
                        int lineCount);
  void emitJumpRequestFromInput();
  void jumpToLineStart();
  void jumpToLineEnd();
  void jumpToDocumentStart();
  void jumpToDocumentEnd();
  void adjustShortcutsAfterToggle(bool checked);
};

#endif // COMMAND_PALETTE_WIDGET_H

#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "utils/gui_utils.h"
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStyle>
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

  void jumpToRowColumn(int currentRow = 0, int currentCol = 0,
                       int lineCount = 1);

signals:
  void goToPositionRequested(int row, int col);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QWidget *parent;
  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;

  const double TOP_OFFSET = 300.0;
  const double SHADOW_X_OFFSET = 0.0;
  const double SHADOW_Y_OFFSET = 5.0;
  const double SHADOW_BLUR_RADIUS = 25.0;
  const double CONTENT_MARGIN = 20.0; // Content margin for drop shadow
  const double WIDTH = 800.0;
  const double HEIGHT = 300.0;

  QFrame *mainFrame = nullptr;
  QVBoxLayout *frameLayout = nullptr;
  QLineEdit *jumpInput = nullptr;

  enum class Mode { None, GoToPosition };
  Mode currentMode = Mode::None;

  void clearContent();
  void buildJumpContent(int currentRow, int currentCol, int lineCount);
  void emitJumpRequestFromInput();
};

#endif // COMMAND_PALETTE_WIDGET_H

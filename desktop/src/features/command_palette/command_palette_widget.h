#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/palette_divider.h"
#include "features/command_palette/palette_frame.h"
#include "utils/gui_utils.h"
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QShortcut>
#include <QStringList>
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
                       int lineCount = 1, int lastLineMaxCol = 1);

signals:
  void goToPositionRequested(int row, int col);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QWidget *parent;
  QWidget *shortcutsContainer;
  QToolButton *shortcutsToggle;
  QShortcut *shortcutsToggleShortcut;
  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;

  PaletteFrame *mainFrame;
  QVBoxLayout *frameLayout;
  QLineEdit *jumpInput;
  QLabel *jumpPlaceholderHint;
  QStringList jumpHistory;
  QString jumpInputDraft;
  int maxLineCount = 1;
  int maxColumn = 1;
  int lastLineMaxColumn = 1;
  int maxRow = 1;
  int currentRow = 1;
  int currentColumn = 1;
  int jumpHistoryIndex = 0;
  bool showJumpShortcuts = false;

  enum class Mode { None, GoToPosition };
  Mode currentMode = Mode::None;

  void clearContent();
  void prepareJumpState(int currentRow, int currentCol, int maxCol,
                        int lineCount, int lastLineMaxCol);
  void buildJumpContent(int currentRow, int currentCol, int maxCol,
                        int lineCount, int lastLineMaxCol);
  void emitJumpRequestFromInput();
  void jumpToLineStart();
  void jumpToLineEnd();
  void jumpToLineMiddle();
  void jumpToDocumentStart();
  void jumpToDocumentEnd();
  void jumpToDocumentMiddle();
  void jumpToDocumentQuarter();
  void jumpToDocumentThreeQuarters();
  void jumpToLastTarget();
  void adjustShortcutsAfterToggle(bool checked);

  struct PaletteColors {
    QString foreground;
    QString foregroundVeryMuted;
    QString border;
  };

  PaletteColors loadPaletteColors() const;
  QFont makeInterfaceFont(qreal pointSize) const;
  void addSpacer(int height);
  void addDivider(const QString &borderColor);
  void addJumpInputRow(int clampedRow, int clampedCol,
                       const PaletteColors &paletteColors, QFont &font);
  void addCurrentLineLabel(int clampedRow, int clampedCol,
                           const PaletteColors &paletteColors, QFont font);
  void addShortcutsSection(const PaletteColors &paletteColors,
                           const QFont &font);
  void updateJumpPlaceholderHint(const QString &placeholder);
  void saveJumpHistoryEntry(const QString &entry);
  void resetJumpHistoryNavigation();
  bool handleJumpHistoryNavigation(QKeyEvent *event);

  static constexpr double TOP_OFFSET = 300.0;
  static constexpr double SHADOW_X_OFFSET = 0.0;
  static constexpr double SHADOW_Y_OFFSET = 5.0;
  static constexpr double SHADOW_BLUR_RADIUS = 25.0;
  static constexpr double CONTENT_MARGIN =
      20.0; // Content margin for drop shadow
  static constexpr double WIDTH = 800.0;
  static constexpr int MIN_WIDTH = 360;

  static constexpr int TOP_SPACER_HEIGHT = 8;
  static constexpr int LABEL_TOP_SPACER_HEIGHT = 4;
  static constexpr int LABEL_BOTTOM_SPACER_HEIGHT = 12;

  static constexpr qreal JUMP_FONT_SIZE = 20.0;
  static constexpr qreal LABEL_FONT_SIZE = 18.0;

  static constexpr char JUMP_INPUT_STYLE[] =
      "color: %1; border: 0px; background: transparent; padding-left: 12px; "
      "padding-right: 12px;";
  static constexpr char LABEL_STYLE[] =
      "color: %1; border: 0px; padding-left: 12px; padding-right: 12px;";
  static constexpr char SHORTCUTS_BUTTON_STYLE[] =
      "QToolButton { color: %1; border: none; background: transparent; "
      "padding-left: 16px; padding-right: 16px; }"
      "QToolButton:hover { color: %2; }";
  static constexpr char SHORTCUTS_BUTTON_TEXT[] = "  Shortcuts";
  static constexpr int JUMP_HISTORY_LIMIT = 20;
  static constexpr char LINE_END_SHORTCUT[] = "le";
  static constexpr char LINE_START_SHORTCUT[] = "lb";
  static constexpr char LINE_MIDDLE_SHORTCUT[] = "lm";
  static constexpr char DOCUMENT_QUARTER_SHORTCUT[] = "dh";
  static constexpr char DOCUMENT_THREE_QUARTERS_SHORTCUT[] = "dt";
  static constexpr char DOCUMENT_END_SHORTCUT[] = "de";
  static constexpr char DOCUMENT_START_SHORTCUT[] = "db";
  static constexpr char DOCUMENT_MIDDLE_SHORTCUT[] = "dm";
  static constexpr char LAST_TARGET_SHORTCUT[] = "ls";
  static constexpr char JUMP_PLACEHOLDER_HINT[] = "↑↓ History";
};

#endif // COMMAND_PALETTE_WIDGET_H

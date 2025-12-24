#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/palette_divider.h"
#include "features/command_palette/palette_frame.h"
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QShortcut>
#include <QStringList>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <neko-core/src/ffi/bridge.rs.h>

class CommandPaletteWidget : public QWidget {
  Q_OBJECT

public:
  explicit CommandPaletteWidget(neko::ThemeManager &themeManager,
                                neko::ConfigManager &configManager,
                                QWidget *parent = nullptr);
  ~CommandPaletteWidget() override = default;

  void applyTheme();
  void showPalette();
  void jumpToRowColumn(int currentRow = 0, int currentCol = 0, int maxCol = 1,
                       int lineCount = 1, int lastLineMaxCol = 1);

signals:
  void goToPositionRequested(int row, int col);
  void commandRequested(const QString &command);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  enum class Mode : std::uint8_t { None, GoToPosition };

  struct PaletteColors {
    QString foreground;
    QString foregroundVeryMuted;
    QString border;
    QString accent;
    QString accentForeground;
  };

  using NavFn = void (CommandPaletteWidget::*)();
  struct NavEntry {
    std::string_view key;
    NavFn fn;
  };

  void connectSignals();
  void clearContent();
  void adjustPosition();
  void prepareJumpState(int currentRow, int currentCol, int maxCol,
                        int lineCount, int lastLineMaxCol);
  void buildJumpContent(int currentRow, int currentCol, int maxCol,
                        int lineCount, int lastLineMaxCol);
  void buildCommandPalette();
  void emitCommandRequestFromInput();
  void emitJumpRequestFromInput();
  void jumpToLineStart();
  void jumpToLineEnd();
  void jumpToLineMiddle();
  void jumpToDocumentStart();
  void jumpToDocumentMiddle();
  void jumpToDocumentQuarter();
  void jumpToDocumentThreeQuarters();
  void jumpToDocumentEnd();
  void jumpToLastTarget();
  void adjustShortcutsAfterToggle(bool checked);

  [[nodiscard]] PaletteColors loadPaletteColors() const;
  [[nodiscard]] QFont makeInterfaceFont(qreal pointSize) const;
  void addSpacer(int height);
  PaletteDivider *addDivider(const QString &borderColor);
  void addJumpInputRow(int clampedRow, int clampedCol,
                       const PaletteColors &paletteColors, QFont &font);
  void addCommandInputRow(const PaletteColors &paletteColors, QFont &font);
  void addCommandSuggestionsList(const PaletteColors &paletteColors,
                                 const QFont &font);
  void addCurrentLineLabel(int clampedRow, int clampedCol,
                           const PaletteColors &paletteColors, QFont font);
  void addShortcutsSection(const PaletteColors &paletteColors,
                           const QFont &font);
  void updateHistoryHint(QWidget *targetInput, const QString &placeholder);
  void createHistoryHint(QWidget *targetInput,
                         const PaletteColors &paletteColors, const QFont &font);
  void saveCommandHistoryEntry(const QString &entry);
  void saveJumpHistoryEntry(const QString &entry);
  void resetJumpHistoryNavigation();
  bool handleJumpHistoryNavigation(const QKeyEvent *event);
  void resetCommandHistoryNavigation();
  bool handleCommandHistoryNavigation(const QKeyEvent *event);
  bool handleCommandSuggestionNavigation(const QKeyEvent *event);
  void updateCommandSuggestions(const QString &text);

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;

  QWidget *parent;
  QWidget *shortcutsContainer;
  QToolButton *shortcutsToggle;
  QShortcut *shortcutsToggleShortcut;
  PaletteFrame *mainFrame;
  QVBoxLayout *frameLayout;
  QLabel *historyHint;
  QLineEdit *jumpInput;
  QLineEdit *commandInput;
  QListWidget *commandSuggestions;
  QStringList jumpHistory;
  QStringList commandHistory;
  QString jumpInputDraft;
  QString commandInputDraft;
  PaletteDivider *commandPaletteBottomDivider;

  int maxLineCount = 1;
  int maxColumn = 1;
  int lastLineMaxColumn = 1;
  int maxRow = 1;
  int currentRow = 1;
  int currentColumn = 1;
  int jumpHistoryIndex = 0;
  int commandHistoryIndex = 0;
  bool showJumpShortcuts = false;
  bool currentlyInHistory = false;
  Mode currentMode = Mode::None;

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

  static constexpr char JUMP_INPUT_STYLE[] = // NOLINT
      "color: %1; border: 0px; background: transparent; padding-left: 12px; "
      "padding-right: 12px;";
  static constexpr char LABEL_STYLE[] = // NOLINT
      "color: %1; border: 0px; padding-left: 0px; padding-right: 0px;";
  static constexpr char SHORTCUTS_BUTTON_STYLE[] = // NOLINT
      "QToolButton { color: %1; border: none; background: transparent; "
      "padding-left: 16px; padding-right: 16px; }"
      "QToolButton:hover { color: %2; }";

  static constexpr char COMMAND_SUGGESTION_STYLE[] = // NOLINT
      "QListWidget { background: transparent; border: none; padding-left: 8px; "
      "padding-right: 8px; }"
      "QListWidget::item { padding: 6px 8px; color: %1; border-radius: 6px; }"
      "QListWidget::item:selected { background: %2; color: %3; }";

  // TODO(scarlet): Move to rust
  // TODO(scarlet): Add autocomplete/correct
  static constexpr char TOGGLE_FILE_EXPLORER_COMMAND[] = // NOLINT
      "file explorer: toggle";
  static constexpr char SET_THEME_TO_LIGHT[] = "set theme: light"; // NOLINT
  static constexpr char SET_THEME_TO_DARK[] = "set theme: dark";   // NOLINT

  static constexpr int JUMP_HISTORY_LIMIT = 20;
  static constexpr int COMMAND_HISTORY_LIMIT = 20;
  static constexpr char SHORTCUTS_BUTTON_TEXT[] = "  Shortcuts"; // NOLINT
  static constexpr char HISTORY_HINT[] = "↑↓ History";           // NOLINT
  static constexpr char COMMAND_PLACEHOLDER_TEXT[] =             // NOLINT
      "Enter a command";

  static constexpr double FRAME_LAYOUT_SPACING = 8.0;
  static constexpr double DASH_LABEL_WIDTH_DIVIDER = 1.93;
  static constexpr double CODE_LABEL_WIDTH_DIVIDER = 1.5;
  static constexpr double COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN = 16.0;
  static constexpr double SHORTCUTS_ROW_SPACING = 6.0;
  static constexpr double COMMAND_INPUT_WIDTH_DIVIDER = 1.25;
  static constexpr double JUMP_INPUT_WIDTH_DIVIDER = 1.5;

  inline static const QStringList AVAILABLE_COMMANDS = {
      TOGGLE_FILE_EXPLORER_COMMAND,
      SET_THEME_TO_LIGHT,
      SET_THEME_TO_DARK,
  };

  static constexpr int JUMP_TO_LAST_TARGET_INDEX = 8;
  static constexpr std::array<NavEntry, 9> NAV = {{
      {"lb", &CommandPaletteWidget::jumpToLineStart},
      {"lm", &CommandPaletteWidget::jumpToLineMiddle},
      {"le", &CommandPaletteWidget::jumpToLineEnd},
      {"db", &CommandPaletteWidget::jumpToDocumentStart},
      {"dm", &CommandPaletteWidget::jumpToDocumentMiddle},
      {"de", &CommandPaletteWidget::jumpToDocumentEnd},
      {"dh", &CommandPaletteWidget::jumpToDocumentQuarter},
      {"dt", &CommandPaletteWidget::jumpToDocumentThreeQuarters},
      {"ls", &CommandPaletteWidget::jumpToLastTarget},
  }};
};

#endif // COMMAND_PALETTE_WIDGET_H

#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/command_palette_mode.h"
#include "features/command_palette/current_size_stacked_widget.h"
#include "features/command_palette/palette_divider.h"
#include "features/command_palette/palette_frame.h"
#include "theme/theme_types.h"
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
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <neko-core/src/ffi/bridge.rs.h>

class CommandPaletteWidget : public QWidget {
  Q_OBJECT

private:
  struct JumpState {
    int maxLineCount = 1;
    int maxColumn = 1;
    int lastLineMaxColumn = 1;
    int maxRow = 1;
    int currentRow = 1;
    int currentColumn = 1;
  };

  static QSpacerItem *buildSpacer(int height);
  static PaletteDivider *buildDivider(QWidget *parent,
                                      const QString &borderColor);

public:
  explicit CommandPaletteWidget(const CommandPaletteTheme &theme,
                                neko::ConfigManager &configManager,
                                QWidget *parent = nullptr);
  ~CommandPaletteWidget() override = default;

  void setAndApplyTheme(const CommandPaletteTheme &newTheme);
  void showPalette(const CommandPaletteMode &mode, JumpState newJumpState);

signals:
  void goToPositionRequested(int row, int col);
  void commandRequested(const QString &command);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  using NavFn = void (CommandPaletteWidget::*)();
  struct NavEntry {
    std::string_view key;
    NavFn fn;
  };

  struct HistoryState {
    QStringList jumpHistory;
    QStringList commandHistory;
    QString jumpInputDraft;
    QString commandInputDraft;
    int jumpHistoryIndex = 0;
    int commandHistoryIndex = 0;
    bool currentlyInHistory = false;
  };

  enum class CommandId : uint8_t { ToggleFileExplorer, ThemeLight, ThemeDark };

  struct CommandDef {
    CommandId id;
    QString displayString;
  };

  inline static const std::array<CommandDef, 3> COMMANDS = {{
      {CommandId::ToggleFileExplorer, QStringLiteral("file explorer: toggle")},
      {CommandId::ThemeLight, QStringLiteral("set theme: light")},
      {CommandId::ThemeDark, QStringLiteral("set theme: dark")},
  }};

  void setUpWindow();
  void buildUi();
  void connectSignals();
  void buildJumpPage();
  void buildCommandPage();
  void adjustPosition();
  QWidget *buildShortcutsContainer(QWidget *parent);
  QWidget *buildShortcutsRow(QWidget *parent);
  void buildShortcutsSection(QLayout *parentLayout, const QFont &font);
  QListWidget *buildCommandSuggestionsList(QWidget *parent, const QFont &font);
  void buildHistoryHint(QWidget *targetInput, const QFont &font);
  void updateHistoryHint(QWidget *targetInput, const QString &placeholder);
  QLabel *buildCurrentLineLabel(QWidget *parent, QFont font) const;

  void emitCommandRequestFromInput();
  void emitJumpRequestFromInput();

  void updateJumpUiFromState();
  void prepareJumpState(int currentRow, int currentCol, int maxCol,
                        int lineCount, int lastLineMaxCol);
  void adjustShortcutsAfterToggle(bool checked);

  void jumpToLineStart();
  void jumpToLineEnd();
  void jumpToLineMiddle();
  void jumpToDocumentStart();
  void jumpToDocumentMiddle();
  void jumpToDocumentQuarter();
  void jumpToDocumentThreeQuarters();
  void jumpToDocumentEnd();
  void jumpToLastTarget();

  [[nodiscard]] QFont makeInterfaceFont(qreal pointSize) const;

  bool handleJumpHistoryNavigation(const QKeyEvent *event);
  void saveJumpHistoryEntry(const QString &entry);
  void resetJumpHistoryNavigation();

  bool handleCommandHistoryNavigation(const QKeyEvent *event);
  void saveCommandHistoryEntry(const QString &entry);
  bool handleCommandSuggestionNavigation(const QKeyEvent *event);
  void updateCommandSuggestions(const QString &text);
  void resetCommandHistoryNavigation();

  neko::ConfigManager &configManager;

  CommandPaletteTheme theme;

  CurrentSizeStackedWidget *pages;
  PaletteFrame *mainFrame;
  QVBoxLayout *frameLayout;
  QWidget *commandPage;
  QWidget *jumpPage;
  QLabel *currentLineLabel;
  QWidget *shortcutsContainer;
  QToolButton *shortcutsToggle;
  QShortcut *shortcutsToggleShortcut;
  QLabel *historyHint;
  QLineEdit *jumpInput;
  QLineEdit *commandInput;
  QListWidget *commandSuggestions;
  PaletteDivider *commandPaletteBottomDivider;

  JumpState jumpState;
  HistoryState historyState;
  CommandPaletteMode currentMode = CommandPaletteMode::Command;
  bool showJumpShortcuts = false;

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
      "QListWidget { background: transparent; border: none; padding-left: "
      "8px; "
      "padding-right: 8px; }"
      "QListWidget::item { padding: 6px 8px; color: %1; border-radius: 6px; }"
      "QListWidget::item:selected { background: %2; color: %3; }";

  // TODO(scarlet): Move to rust
  // TODO(scarlet): Add autocomplete/correct
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

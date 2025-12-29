#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/types/types.h"
#include "theme/types/types.h"
#include "types/qt_types_fwd.h"
#include <QStringList>
#include <QWidget>

QT_FWD(QVBoxLayout, QLabel, QLineEdit, QListWidget, QToolButton, QShortcut,
       QSpacerItem, QShowEvent, QEvent, QKeyEvent, QShowEvent);

class PaletteDivider;
class PaletteFrame;
class CurrentSizeStackedWidget;

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

  using NavFn = void (CommandPaletteWidget::*)();
  struct NavEntry {
    std::string_view key;
    NavFn fn;
  };

  static QSpacerItem *buildSpacer(int height);
  static PaletteDivider *buildDivider(QWidget *parent,
                                      const QString &borderColor);
  static void setSpacerHeight(QSpacerItem *spacer, int height);
  static void setVisibilityIfNotNull(QWidget *widget, bool visible);

public:
  struct CommandPaletteProps {
    QFont font;
    CommandPaletteTheme theme;
    std::vector<ShortcutHintRow> jumpHints;
  };

  explicit CommandPaletteWidget(const CommandPaletteProps &props,
                                QWidget *parent = nullptr);
  ~CommandPaletteWidget() override = default;

  void setAndApplyTheme(const CommandPaletteTheme &newTheme);
  void showPalette(const CommandPaletteMode &mode, JumpState newJumpState);

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void updateFont(const QFont &newFont);

signals:
  void goToPositionRequested(const QString &jumpCommand, int64_t row,
                             int64_t column, bool isPosition);
  void commandRequested(const QString &command);

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  struct HistoryState {
    QStringList jumpHistory;
    QStringList commandHistory;
    QString jumpInputDraft;
    QString commandInputDraft;
    int jumpHistoryIndex = 0;
    int commandHistoryIndex = 0;
    bool currentlyInHistory = false;
  };

  static constexpr std::string_view kLastJumpKey = "ls";

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

  void processJumpAndClose(const QString &jumpCommand);

  bool handleJumpHistoryNavigation(const QKeyEvent *event);
  void saveJumpHistoryEntry(const QString &entry);
  void resetJumpHistoryNavigation();

  bool handleCommandHistoryNavigation(const QKeyEvent *event);
  void saveCommandHistoryEntry(const QString &entry);
  bool handleCommandSuggestionNavigation(const QKeyEvent *event);
  bool selectNextCommandSuggestion();
  bool selectPreviousCommandSuggestion();
  bool applyCurrentCommandSuggestion();
  bool canHandleSuggestionNav(const QKeyEvent *event) const;
  [[nodiscard]] int clampSuggestionRow(int row) const;
  void setSuggestionRowClamped(int row);
  void updateCommandSuggestions(const QString &text);
  void resetCommandHistoryNavigation();

  CommandPaletteTheme theme;
  QFont font;
  std::vector<ShortcutHintRow> shortcutRows;

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
  PaletteDivider *commandTopDivider;
  PaletteDivider *jumpTopDivider;
  QSpacerItem *commandBottomSpacer;
  QSpacerItem *commandTopSpacer;

  JumpState jumpState;
  HistoryState historyState;
  CommandPaletteMode currentMode = CommandPaletteMode::Command;
  bool showJumpShortcuts = false;
};

#endif // COMMAND_PALETTE_WIDGET_H

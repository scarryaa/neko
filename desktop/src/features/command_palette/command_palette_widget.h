#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include "features/command_palette/command_palette_mode.h"
#include "theme/theme_types.h"
#include <QStringList>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QShortcut)
QT_FORWARD_DECLARE_CLASS(QSpacerItem)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)

class PaletteDivider;
class PaletteFrame;
class CurrentSizeStackedWidget;

namespace neko {
class ConfigManager;
}

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
  static void setVisibleIf(QWidget *widget, bool visible);
  static const NavEntry *findNav(std::string_view key);

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
  struct HistoryState {
    QStringList jumpHistory;
    QStringList commandHistory;
    QString jumpInputDraft;
    QString commandInputDraft;
    int jumpHistoryIndex = 0;
    int commandHistoryIndex = 0;
    bool currentlyInHistory = false;
  };

  enum class ApplySuggestionMode : uint8_t { FillOnly, FillAndRun };

  static constexpr std::string_view kLastJumpKey = "ls";
  static constexpr int kNavCount = 9;
  static const std::array<NavEntry, kNavCount> &navTable();

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
  bool selectNextCommandSuggestion();
  bool selectPreviousCommandSuggestion();
  bool applyCurrentCommandSuggestion(ApplySuggestionMode mode);
  bool canHandleSuggestionNav(const QKeyEvent *event) const;
  [[nodiscard]] int clampSuggestionRow(int row) const;
  void setSuggestionRowClamped(int row);
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

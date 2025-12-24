#include "command_palette_widget.h"
#include "features/command_palette/command_palette_mode.h"
#include "features/command_palette/current_size_stacked_widget.h"
#include "features/command_palette/palette_divider.h"
#include "features/command_palette/palette_frame.h"
#include "theme/theme_types.h"
#include "utils/gui_utils.h"
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

namespace {
enum class CommandId : uint8_t { ToggleFileExplorer, ThemeLight, ThemeDark };

struct CommandDef {
  CommandId id;
  QString displayString;
};

inline const std::array<CommandDef, 3> COMMANDS = {{
    {CommandId::ToggleFileExplorer, QStringLiteral("file explorer: toggle")},
    {CommandId::ThemeLight, QStringLiteral("set theme: light")},
    {CommandId::ThemeDark, QStringLiteral("set theme: dark")},
}};

namespace k {
constexpr double TOP_OFFSET = 300.0;
constexpr double SHADOW_X_OFFSET = 0.0;
constexpr double SHADOW_Y_OFFSET = 5.0;
constexpr double SHADOW_BLUR_RADIUS = 25.0;
constexpr double CONTENT_MARGIN = 20.0; // Content margin for drop shadow
constexpr double WIDTH = 800.0;
constexpr int MIN_WIDTH = 360;

constexpr int TOP_SPACER_HEIGHT = 8;
constexpr int LABEL_TOP_SPACER_HEIGHT = 4;
constexpr int LABEL_BOTTOM_SPACER_HEIGHT = 12;

constexpr qreal JUMP_FONT_SIZE = 20.0;
constexpr qreal LABEL_FONT_SIZE = 18.0;

constexpr char HISTORY_HINT[] = "↑↓ History"; // NOLINT
constexpr char COMMAND_PLACEHOLDER_TEXT[] =   // NOLINT
    "Enter a command";

constexpr int JUMP_HISTORY_LIMIT = 20;
constexpr int COMMAND_HISTORY_LIMIT = 20;
constexpr char SHORTCUTS_BUTTON_TEXT[] = "  Shortcuts"; // NOLINT

constexpr double FRAME_LAYOUT_SPACING = 8.0;
constexpr double DASH_LABEL_WIDTH_DIVIDER = 1.93;
constexpr double CODE_LABEL_WIDTH_DIVIDER = 1.5;
constexpr double COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN = 16.0;
constexpr double SHORTCUTS_ROW_SPACING = 6.0;
constexpr double COMMAND_INPUT_WIDTH_DIVIDER = 1.25;
constexpr double JUMP_INPUT_WIDTH_DIVIDER = 1.5;

constexpr char JUMP_INPUT_STYLE[] = // NOLINT
    "color: %1; border: 0px; background: transparent; padding-left: 12px; "
    "padding-right: 12px;";
constexpr char LABEL_STYLE[] = // NOLINT
    "color: %1; border: 0px; padding-left: 0px; padding-right: 0px;";
constexpr char SHORTCUTS_BUTTON_STYLE[] = // NOLINT
    "QToolButton { color: %1; border: none; background: transparent; "
    "padding-left: 16px; padding-right: 16px; }"
    "QToolButton:hover { color: %2; }";

constexpr char COMMAND_SUGGESTION_STYLE[] = // NOLINT
    "QListWidget { background: transparent; border: none; padding-left: "
    "8px; "
    "padding-right: 8px; }"
    "QListWidget::item { padding: 6px 8px; color: %1; border-radius: 6px; }"
    "QListWidget::item:selected { background: %2; color: %3; }";
} // namespace k
} // namespace

const std::array<CommandPaletteWidget::NavEntry,
                 CommandPaletteWidget::kNavCount> &
CommandPaletteWidget::navTable() {
  using NavEntry = CommandPaletteWidget::NavEntry;

  static const std::array<NavEntry, CommandPaletteWidget::kNavCount> kNav = {{
      {"lb", &CommandPaletteWidget::jumpToLineStart},
      {"lm", &CommandPaletteWidget::jumpToLineMiddle},
      {"le", &CommandPaletteWidget::jumpToLineEnd},
      {"db", &CommandPaletteWidget::jumpToDocumentStart},
      {"dm", &CommandPaletteWidget::jumpToDocumentMiddle},
      {"de", &CommandPaletteWidget::jumpToDocumentEnd},
      {"dh", &CommandPaletteWidget::jumpToDocumentQuarter},
      {"dt", &CommandPaletteWidget::jumpToDocumentThreeQuarters},
      {CommandPaletteWidget::kLastJumpKey,
       &CommandPaletteWidget::jumpToLastTarget},
  }};

  return kNav;
}

// Static methods
QSpacerItem *CommandPaletteWidget::buildSpacer(const int height) {
  return new QSpacerItem(0, height, QSizePolicy::Minimum, QSizePolicy::Fixed);
}

PaletteDivider *CommandPaletteWidget::buildDivider(QWidget *parent,
                                                   const QString &borderColor) {
  auto *divider = new PaletteDivider(borderColor, parent);
  divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  divider->setFixedHeight(1);
  divider->setStyleSheet(QString("background-color: %1;").arg(borderColor));

  return divider;
}

void CommandPaletteWidget::setSpacerHeight(QSpacerItem *spacer, int height) {
  if (spacer == nullptr) {
    return;
  }

  spacer->changeSize(0, height, QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void CommandPaletteWidget::setVisibleIf(QWidget *widget, bool visible) {
  if (widget != nullptr) {
    widget->setVisible(visible);
  }
}

const CommandPaletteWidget::NavEntry *
CommandPaletteWidget::findNav(std::string_view key) {
  for (const auto &entry : navTable()) {
    if (entry.key == key) {
      return &entry;
    }
  }

  return nullptr;
}

CommandPaletteWidget::CommandPaletteWidget(const CommandPaletteTheme &theme,
                                           neko::ConfigManager &configManager,
                                           QWidget *parent)
    : QWidget(parent), theme(theme), configManager(configManager) {
  setUpWindow();
  buildUi();
  connectSignals();
  setAndApplyTheme(theme);
}

void CommandPaletteWidget::setUpWindow() {
  setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                 Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAutoFillBackground(false);
  installEventFilter(this);
  setMinimumWidth(k::MIN_WIDTH);
  setMaximumWidth(k::WIDTH);
}

void CommandPaletteWidget::buildUi() {
  mainFrame =
      new PaletteFrame({theme.backgroundColor, theme.borderColor}, this);
  mainFrame->setObjectName("commandPaletteFrame");

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(k::CONTENT_MARGIN, k::CONTENT_MARGIN,
                                 k::CONTENT_MARGIN, k::CONTENT_MARGIN);
  rootLayout->setSizeConstraint(QLayout::SetFixedSize);
  rootLayout->addWidget(mainFrame);

  frameLayout = new QVBoxLayout(mainFrame);
  frameLayout->setSpacing(k::FRAME_LAYOUT_SPACING);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frameLayout->setAlignment(Qt::AlignTop);

  auto *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(k::SHADOW_BLUR_RADIUS);
  shadow->setColor(Qt::black);
  shadow->setOffset(k::SHADOW_X_OFFSET, k::SHADOW_Y_OFFSET);
  mainFrame->setGraphicsEffect(shadow);

  shortcutsToggleShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
  shortcutsToggleShortcut->setContext(Qt::WidgetWithChildrenShortcut);

  pages = new CurrentSizeStackedWidget(mainFrame);
  frameLayout->addWidget(pages);

  connect(pages, &QStackedWidget::currentChanged, this, [this] {
    pages->updateGeometry();
    mainFrame->updateGeometry();
    this->adjustSize();
    adjustPosition();
  });

  buildCommandPage();
  buildJumpPage();

  pages->setCurrentWidget(commandPage);
}

void CommandPaletteWidget::connectSignals() {
  connect(shortcutsToggleShortcut, &QShortcut::activated, this, [this]() {
    if (shortcutsToggle) {
      shortcutsToggle->toggle();
    }
  });

  connect(shortcutsToggle, &QToolButton::toggled, this,
          [this](bool checked) { adjustShortcutsAfterToggle(checked); });

  connect(jumpInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitJumpRequestFromInput);
  connect(jumpInput, &QLineEdit::textEdited, this,
          [this](const QString &) { resetJumpHistoryNavigation(); });
  connect(jumpInput, &QLineEdit::textChanged, this, [this](const QString &) {
    updateHistoryHint(jumpInput, k::HISTORY_HINT);
  });

  connect(commandInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitCommandRequestFromInput);
  connect(commandInput, &QLineEdit::textEdited, this,
          [this](const QString &) { resetCommandHistoryNavigation(); });
  connect(commandInput, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            updateHistoryHint(commandInput, k::HISTORY_HINT);
            updateCommandSuggestions(text);
          });

  connect(commandSuggestions, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            if (!item || !commandInput) {
              return;
            }

            commandInput->setText(item->text());
            commandInput->setCursorPosition(
                static_cast<int>(commandInput->text().length()));
            emitCommandRequestFromInput();
          });
}

void CommandPaletteWidget::buildJumpPage() {
  jumpPage = new QWidget(pages);

  auto font = makeInterfaceFont(k::JUMP_FONT_SIZE);

  auto *layout = new QVBoxLayout(jumpPage);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(k::FRAME_LAYOUT_SPACING);
  layout->setAlignment(Qt::AlignTop);

  // Top spacer
  layout->addItem(buildSpacer(k::TOP_SPACER_HEIGHT));

  // Jump input
  jumpInput = new QLineEdit(jumpPage);
  jumpInput->setFont(font);
  jumpInput->setStyleSheet(
      QString(k::JUMP_INPUT_STYLE).arg(theme.foregroundColor));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  jumpInput->installEventFilter(this);
  jumpInput->setMinimumWidth(
      static_cast<int>(k::WIDTH / k::JUMP_INPUT_WIDTH_DIVIDER));
  layout->addWidget(jumpInput);

  // Divider
  jumpTopDivider = buildDivider(jumpPage, theme.borderColor);
  layout->addWidget(jumpTopDivider);

  // Spacer
  layout->addItem(
      CommandPaletteWidget::buildSpacer(k::LABEL_TOP_SPACER_HEIGHT));

  // Current line label
  currentLineLabel = buildCurrentLineLabel(jumpPage, font);
  layout->addWidget(currentLineLabel);

  // Shortcuts header row
  auto *shortcutsRow = buildShortcutsRow(jumpPage);
  layout->addWidget(shortcutsRow);

  // Shortcuts container
  shortcutsContainer = buildShortcutsContainer(jumpPage);
  layout->addWidget(shortcutsContainer);
  shortcutsContainer->setVisible(showJumpShortcuts);

  // Bottom spacer
  layout->addItem(
      CommandPaletteWidget::buildSpacer(k::LABEL_BOTTOM_SPACER_HEIGHT));

  pages->addWidget(jumpPage);
}

void CommandPaletteWidget::buildCommandPage() {
  commandPage = new QWidget(pages);

  auto font = makeInterfaceFont(k::JUMP_FONT_SIZE);

  auto *layout = new QVBoxLayout(commandPage);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(k::FRAME_LAYOUT_SPACING);

  // Top spacer
  layout->addItem(buildSpacer(k::TOP_SPACER_HEIGHT));

  // Command input
  commandInput = new QLineEdit(commandPage);
  commandInput->setFont(font);
  const QString placeholderText = k::COMMAND_PLACEHOLDER_TEXT;
  commandInput->setPlaceholderText(placeholderText);
  commandInput->setStyleSheet(
      QString(k::JUMP_INPUT_STYLE).arg(theme.foregroundColor));
  commandInput->setClearButtonEnabled(false);
  commandInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  commandInput->installEventFilter(this);
  commandInput->setMinimumWidth(k::WIDTH / k::COMMAND_INPUT_WIDTH_DIVIDER);
  layout->addWidget(commandInput);

  // Divider
  commandTopDivider = buildDivider(commandPage, theme.borderColor);
  layout->addWidget(commandTopDivider);

  // Spacer (when suggestions are empty)
  commandTopSpacer = buildSpacer(k::TOP_SPACER_HEIGHT);
  layout->addSpacerItem(commandTopSpacer);

  // Suggestions
  commandSuggestions = buildCommandSuggestionsList(commandPage, font);
  layout->addWidget(commandSuggestions);

  // Spacer
  commandBottomSpacer = buildSpacer(k::TOP_SPACER_HEIGHT);
  layout->addSpacerItem(commandBottomSpacer);

  pages->addWidget(commandPage);
}

void CommandPaletteWidget::adjustPosition() {
  if (parentWidget() == nullptr) {
    return;
  }

  const int availableWidth = std::max(
      0, parentWidget()->width() - static_cast<int>(k::CONTENT_MARGIN * 2));
  const int xPos = static_cast<int>((parentWidget()->width() - k::WIDTH) / 2.0);
  const int yPos = static_cast<int>(k::TOP_OFFSET);

  move(parentWidget()->mapToGlobal(QPoint(xPos, yPos)));
}

QWidget *CommandPaletteWidget::buildShortcutsContainer(QWidget *parent) {
  const auto font = makeInterfaceFont(k::JUMP_FONT_SIZE);

  struct ShortcutRow {
    std::string_view code;
    const char *desc;
  };

  // NOLINTNEXTLINE
  const ShortcutRow rows[] = {
      {navTable()[0].key, "current line beginning"},  // NOLINT
      {navTable()[1].key, "current line middle"},     // NOLINT
      {navTable()[2].key, "current line end"},        // NOLINT
      {navTable()[3].key, "document beginning"},      // NOLINT
      {navTable()[4].key, "document middle"},         // NOLINT
      {navTable()[5].key, "document end"},            // NOLINT
      {navTable()[6].key, "document quarter"},        // NOLINT
      {navTable()[7].key, "document three-quarters"}, // NOLINT
      {navTable()[8].key, "last jumped-to position"}, // NOLINT
  };

  const QFontMetrics fontMetrics(font);
  int codeColumnWidth = 0;
  for (const auto &row : rows) {
    const QString codeStr =
        QString::fromLatin1(row.code.data(), static_cast<int>(row.code.size()));
    codeColumnWidth =
        std::max(codeColumnWidth, fontMetrics.horizontalAdvance(codeStr));
  }
  codeColumnWidth += fontMetrics.horizontalAdvance("  ");

  shortcutsContainer = new QWidget(parent);
  auto *shortcutsLayout = new QVBoxLayout(shortcutsContainer);
  shortcutsLayout->setContentsMargins(4, 0, 4, 0);
  shortcutsLayout->setSpacing(2);

  const QString hintStyle =
      QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor);
  const auto createHintLabel = [&](const QString &text) {
    return UiUtils::createLabel(text, hintStyle, font, parent, false,
                                QSizePolicy::Fixed, QSizePolicy::Fixed);
  };

  for (const auto &row : rows) {
    auto *rowWidget = new QWidget(shortcutsContainer);
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(k::COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN, 0,
                                  k::COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN, 0);
    rowLayout->setSpacing(4);

    const QString codeStr =
        QString::fromLatin1(row.code.data(), static_cast<int>(row.code.size()));
    auto *codeLabel =
        UiUtils::createLabel(codeStr, hintStyle, font, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);
    codeLabel->setMinimumWidth(
        static_cast<int>(codeColumnWidth / k::CODE_LABEL_WIDTH_DIVIDER));

    auto *dashLabel =
        UiUtils::createLabel("", hintStyle, font, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);
    dashLabel->setMinimumWidth(
        static_cast<int>(codeColumnWidth / k::DASH_LABEL_WIDTH_DIVIDER));
    auto *descLabel =
        UiUtils::createLabel(row.desc, hintStyle, font, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);

    rowLayout->addWidget(codeLabel);
    rowLayout->addWidget(dashLabel);
    rowLayout->addWidget(descLabel);
    rowLayout->addStretch(1);
    shortcutsLayout->addWidget(rowWidget);
  }

  return shortcutsContainer;
}

QWidget *CommandPaletteWidget::buildShortcutsRow(QWidget *parent) {
  QFont font = makeInterfaceFont(k::JUMP_FONT_SIZE);

  auto *shortcutsRow = new QWidget(jumpPage);

  auto *shortcutsRowLayout = new QHBoxLayout(shortcutsRow);
  shortcutsRowLayout->setContentsMargins(0, 2, 0, 0);
  shortcutsRowLayout->setSpacing(k::SHORTCUTS_ROW_SPACING);

  shortcutsToggle = new QToolButton(shortcutsRow);
  shortcutsToggle->setText(k::SHORTCUTS_BUTTON_TEXT);
  shortcutsToggle->setCheckable(true);
  shortcutsToggle->setChecked(showJumpShortcuts);
  shortcutsToggle->setArrowType(Qt::DownArrow);
  shortcutsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  shortcutsToggle->setFont(font);
  shortcutsToggle->setStyleSheet(
      QString(k::SHORTCUTS_BUTTON_STYLE)
          .arg(theme.foregroundColor, theme.foregroundVeryMutedColor));
  shortcutsToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  shortcutsRowLayout->addWidget(shortcutsToggle);

  if (shortcutsToggleShortcut != nullptr) {
    const QString shortcutText =
        shortcutsToggleShortcut->key().toString(QKeySequence::NativeText);

    if (!shortcutText.isEmpty()) {
      auto *shortcutHint = UiUtils::createLabel(
          shortcutText,
          QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
              QString("padding-right: 12px;"),
          font, parent, false, QSizePolicy::Fixed, QSizePolicy::Fixed);
      shortcutHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      shortcutsRowLayout->addWidget(shortcutHint);
    }
  }

  return shortcutsRow;
}

void CommandPaletteWidget::buildShortcutsSection(QLayout *parentLayout,
                                                 const QFont &font) {

  parentLayout->addWidget(shortcutsContainer);
  adjustShortcutsAfterToggle(showJumpShortcuts);
}

QListWidget *
CommandPaletteWidget::buildCommandSuggestionsList(QWidget *parent,
                                                  const QFont &font) {
  auto *suggestions = new QListWidget(parent);
  suggestions->setFont(font);
  suggestions->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  suggestions->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  suggestions->setFrameShape(QFrame::NoFrame);
  suggestions->setSelectionMode(QAbstractItemView::SingleSelection);
  suggestions->setUniformItemSizes(true);
  suggestions->setVisible(false);
  suggestions->setFocusPolicy(Qt::NoFocus);

  const QString suggestionStyle =
      QString(k::COMMAND_SUGGESTION_STYLE)
          .arg(theme.foregroundColor, theme.accentMutedColor,
               theme.accentForegroundColor);
  suggestions->setStyleSheet(suggestionStyle);

  return suggestions;
}

void CommandPaletteWidget::buildHistoryHint(QWidget *targetInput,
                                            const QFont &font) {
  historyHint = UiUtils::createLabel(
      "",
      QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
          "padding-right: 12px;",
      font, targetInput, false, QSizePolicy::Expanding, QSizePolicy::Preferred);
  historyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
  historyHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void CommandPaletteWidget::setAndApplyTheme(
    const CommandPaletteTheme &newTheme) {
  theme = newTheme;

  const QString stylesheet =
      "CommandPaletteWidget { background: transparent; border: none; "
      "} #commandPaletteFrame { border-radius: 12px; background: %1; border: "
      "2px "
      "solid %2; }";
  setStyleSheet(stylesheet.arg(theme.backgroundColor, theme.borderColor));

  if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(
          mainFrame->graphicsEffect())) {
    shadow->setColor(theme.shadowColor);
  }

  mainFrame->setAndApplyTheme({theme.backgroundColor, theme.borderColor});

  // Input styles
  const QString inputStyle =
      QString(k::JUMP_INPUT_STYLE).arg(theme.foregroundColor);

  if (jumpInput != nullptr) {
    jumpInput->setStyleSheet(inputStyle);
  }

  if (commandInput != nullptr) {
    commandInput->setStyleSheet(inputStyle);
  }

  // Suggestions list style
  if (commandSuggestions != nullptr) {
    commandSuggestions->setStyleSheet(QString(k::COMMAND_SUGGESTION_STYLE)
                                          .arg(theme.foregroundColor,
                                               theme.accentMutedColor,
                                               theme.accentForegroundColor));
  }

  // Toggle button style
  if (shortcutsToggle != nullptr) {
    shortcutsToggle->setStyleSheet(
        QString(k::SHORTCUTS_BUTTON_STYLE)
            .arg(theme.foregroundColor, theme.foregroundVeryMutedColor));
  }

  // Dividers
  auto applyDivider = [this](PaletteDivider *divider) {
    if (divider == nullptr) {
      return;
    }

    divider->setStyleSheet(
        QString("background-color: %1;").arg(theme.borderColor));
    divider->update();
  };

  applyDivider(jumpTopDivider);
  applyDivider(commandTopDivider);
  applyDivider(commandPaletteBottomDivider);

  // Labels
  if (currentLineLabel != nullptr) {
    const QString styleSheet =
        QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
        "padding-left: 12px";

    currentLineLabel->setStyleSheet(styleSheet);
  }

  if (historyHint != nullptr) {
    historyHint->setStyleSheet(
        QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
        "padding-right: 12px;");
  }

  if (commandInput != nullptr) {
    updateCommandSuggestions(commandInput->text());
  }

  if (jumpInput != nullptr) {
    updateJumpUiFromState();
  }

  update();
}

void CommandPaletteWidget::showPalette(const CommandPaletteMode &mode,
                                       JumpState newJumpState) {
  currentMode = mode;

  if (mode == CommandPaletteMode::Command) {
    pages->setCurrentWidget(commandPage);
    resetCommandHistoryNavigation();
    commandInput->setText("");

    updateCommandSuggestions(commandInput->text());
    show();

    commandInput->setFocus();
  } else if (mode == CommandPaletteMode::Jump) {
    prepareJumpState(newJumpState.currentRow, newJumpState.currentColumn,
                     newJumpState.maxColumn, newJumpState.maxLineCount,
                     newJumpState.lastLineMaxColumn);
    updateJumpUiFromState();

    resetJumpHistoryNavigation();
    jumpInput->setText("");

    pages->setCurrentWidget(jumpPage);
    show();

    jumpInput->setFocus();
  }
}

void CommandPaletteWidget::showEvent(QShowEvent *event) {
  if (parentWidget() == nullptr) {
    return;
  }

  adjustPosition();
  QWidget::showEvent(event);
}

bool CommandPaletteWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == jumpInput && event->type() == QEvent::KeyPress) {
    const auto *keyEvent = static_cast<QKeyEvent *>(event);

    if (handleJumpHistoryNavigation(keyEvent)) {
      return true;
    }
  }

  if (obj == commandInput && event->type() == QEvent::KeyPress) {
    const auto *keyEvent = static_cast<QKeyEvent *>(event);

    if (handleCommandSuggestionNavigation(keyEvent)) {
      return true;
    }

    if (handleCommandHistoryNavigation(keyEvent)) {
      return true;
    }
  }

  if (event->type() == QEvent::Resize) {
    if (obj == jumpInput) {
      updateHistoryHint(jumpInput, k::HISTORY_HINT);
    } else if (obj == commandInput) {
      updateHistoryHint(commandInput, k::HISTORY_HINT);
    }
  }

  if (event->type() == QEvent::MouseButtonPress) {
    auto *mouseEvent = static_cast<QMouseEvent *>(event);

    if (!this->rect().contains(
            this->mapFromGlobal(mouseEvent->globalPosition().toPoint()))) {
      this->close();
      return true;
    }
  }

  return QWidget::eventFilter(obj, event);
}

void CommandPaletteWidget::updateHistoryHint(QWidget *targetInput,
                                             const QString &placeholder) {
  if ((targetInput == nullptr) || (historyHint == nullptr)) {
    return;
  }

  historyHint->setText(placeholder);
  const bool shouldShow =
      static_cast<QLineEdit *>(targetInput)->text().isEmpty();
  historyHint->setVisible(shouldShow);
  historyHint->setGeometry(targetInput->rect().adjusted(0, 0, 0, 0));
}

void CommandPaletteWidget::emitCommandRequestFromInput() {
  if (commandInput == nullptr) {
    return;
  }

  const QString command = commandInput->text().trimmed();
  if (command.isEmpty()) {
    close();
    return;
  }

  // TODO(scarlet): Allow aliases?
  const bool commandIsKnown =
      std::any_of(COMMANDS.begin(), COMMANDS.end(),
                  [&command](const CommandDef &commandDef) {
                    return commandDef.displayString == command;
                  });

  if (commandIsKnown) {
    saveCommandHistoryEntry(command);
    emit commandRequested(command);
  }

  close();
}

void CommandPaletteWidget::emitJumpRequestFromInput() {
  if (jumpInput == nullptr) {
    return;
  }

  const QString text = jumpInput->text().trimmed();
  if (text.isEmpty()) {
    close();
    return;
  }

  const QStringList parts = text.split(':', Qt::SkipEmptyParts);
  bool okRow = false;
  bool okCol = true;

  const auto storeEntry = [this](const QString &entry) {
    saveJumpHistoryEntry(entry);
  };

  // TODO(scarlet): Allow aliases?
  const QString &value = text;

  if (const auto *entry = findNav(text.toStdString())) {
    if (entry->key != kLastJumpKey) {
      storeEntry(text);
    }

    (this->*(entry->fn))();
    return;
  }

  int row = parts.value(0).toInt(&okRow);
  int col = parts.size() > 1 ? parts.value(1).toInt(&okCol) : 1;

  if (!okRow || !okCol || row < 0 || col < 0) {
    close();
    return;
  }

  const int effectiveMaxLine = std::max(1, jumpState.maxLineCount);
  row = std::clamp(row, 1, effectiveMaxLine);
  col = std::max(1, col);

  storeEntry(text);
  emit goToPositionRequested(row - 1, col - 1);
  close();
}

void CommandPaletteWidget::updateJumpUiFromState() {
  if ((jumpInput == nullptr) || (currentLineLabel == nullptr)) {
    return;
  }

  const int row0 = jumpState.currentRow;
  const int col0 = std::clamp(jumpState.currentColumn, 0,
                              std::max(0, jumpState.maxColumn - 1));

  jumpInput->setPlaceholderText(QString("%1:%2").arg(row0 + 1).arg(col0 + 1));

  currentLineLabel->setText(QString("Current line: %1 of %2 (column %3)")
                                .arg(row0 + 1)
                                .arg(jumpState.maxLineCount)
                                .arg(col0 + 1));
}

void CommandPaletteWidget::prepareJumpState(const int currentRow,
                                            const int currentCol,
                                            const int maxCol,
                                            const int lineCount,
                                            const int lastLineMaxCol) {
  const int maxLineCount = std::max(1, lineCount);
  const int maxColumn = std::max(1, maxCol);
  const int lastLineMaxColumn = std::max(1, lastLineMaxCol);
  const int maxRow = std::max(1, lineCount);
  const int clampedCurrentRow = std::clamp(currentRow, 0, maxLineCount - 1);
  const int clampedCurrentColumn = std::clamp(currentCol, 0, maxCol);

  JumpState newJumpState{maxLineCount, maxColumn,         lastLineMaxColumn,
                         maxRow,       clampedCurrentRow, clampedCurrentColumn};
  jumpState = newJumpState;
}

void CommandPaletteWidget::adjustShortcutsAfterToggle(const bool checked) {
  if ((shortcutsContainer == nullptr) || (shortcutsToggle == nullptr)) {
    return;
  }

  shortcutsContainer->setVisible(checked);
  shortcutsToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
  shortcutsContainer->adjustSize();
  shortcutsToggle->updateGeometry();
  shortcutsContainer->updateGeometry();
  showJumpShortcuts = checked;
  this->adjustSize();
}

// TODO(scarlet): Consolidate these?
void CommandPaletteWidget::jumpToLineStart() {
  emit goToPositionRequested(jumpState.currentRow, 0);
  close();
}

void CommandPaletteWidget::jumpToLineEnd() {
  emit goToPositionRequested(jumpState.currentRow, jumpState.maxColumn);
  close();
}

void CommandPaletteWidget::jumpToLineMiddle() {
  emit goToPositionRequested(jumpState.currentRow, jumpState.maxColumn / 2);
  close();
}

void CommandPaletteWidget::jumpToDocumentStart() {
  emit goToPositionRequested(0, 0);
  close();
}

void CommandPaletteWidget::jumpToDocumentMiddle() {
  emit goToPositionRequested(jumpState.maxRow / 2, jumpState.currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentQuarter() {
  emit goToPositionRequested(jumpState.maxRow / 4, jumpState.currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentThreeQuarters() {
  emit goToPositionRequested((jumpState.maxRow / 4) * 3,
                             jumpState.currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentEnd() {
  emit goToPositionRequested(jumpState.maxRow, jumpState.lastLineMaxColumn);
  close();
}

void CommandPaletteWidget::jumpToLastTarget() {
  if ((jumpInput == nullptr) || historyState.jumpHistory.isEmpty()) {
    close();
    return;
  }

  const QString lsKey = QString::fromLatin1(
      kLastJumpKey.data(), static_cast<int>(kLastJumpKey.size()));
  for (int i = static_cast<int>(historyState.jumpHistory.size()) - 1; i >= 0;
       --i) {
    const auto &entry = historyState.jumpHistory.at(i);
    if (entry.trimmed() == lsKey) {
      continue;
    }

    jumpInput->setText(entry);
    emitJumpRequestFromInput();
    return;
  }

  close();
}

QFont CommandPaletteWidget::makeInterfaceFont(const qreal pointSize) const {
  auto font = UiUtils::loadFont(configManager, neko::FontType::Interface);
  font.setPointSizeF(pointSize);
  return font;
}

QLabel *CommandPaletteWidget::buildCurrentLineLabel(QWidget *parent,
                                                    QFont font) const {
  font.setPointSizeF(k::LABEL_FONT_SIZE);
  const QString styleSheet =
      QString(k::LABEL_STYLE).arg(theme.foregroundVeryMutedColor);
  auto *label =
      UiUtils::createLabel("", styleSheet + "padding-left: 12px;", font, parent,
                           false, QSizePolicy::Fixed, QSizePolicy::Fixed);

  return label;
}

bool CommandPaletteWidget::handleJumpHistoryNavigation(const QKeyEvent *event) {
  if (jumpInput == nullptr) {
    return false;
  }

  Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier) {
    return false;
  }

  if (historyState.jumpHistory.isEmpty()) {
    return false;
  }

  const auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < historyState.jumpHistory.size()) {
      jumpInput->setText(historyState.jumpHistory.at(index));
    } else {
      jumpInput->setText(historyState.jumpInputDraft);
    }
    jumpInput->setCursorPosition(static_cast<int>(jumpInput->text().length()));
  };

  if (event->key() == Qt::Key_Up) {
    if (historyState.jumpHistoryIndex == historyState.jumpHistory.size()) {
      historyState.jumpInputDraft = jumpInput->text();
    }

    historyState.jumpHistoryIndex =
        std::max(0, historyState.jumpHistoryIndex - 1);
    applyHistoryEntry(historyState.jumpHistoryIndex);
    return true;
  }

  if (event->key() == Qt::Key_Down) {
    if (historyState.jumpHistoryIndex == historyState.jumpHistory.size()) {
      historyState.jumpInputDraft = jumpInput->text();
    }

    historyState.jumpHistoryIndex =
        std::min(historyState.jumpHistoryIndex + 1,
                 static_cast<int>(historyState.jumpHistory.size()));
    applyHistoryEntry(historyState.jumpHistoryIndex);
    return true;
  }

  return false;
}

void CommandPaletteWidget::saveJumpHistoryEntry(const QString &entry) {
  if (entry.isEmpty()) {
    return;
  }

  if (historyState.jumpHistory.isEmpty() ||
      historyState.jumpHistory.back() != entry) {
    historyState.jumpHistory.append(entry);
    if (historyState.jumpHistory.size() > k::JUMP_HISTORY_LIMIT) {
      historyState.jumpHistory.removeFirst();
    }
  }

  resetJumpHistoryNavigation();
}

void CommandPaletteWidget::resetJumpHistoryNavigation() {
  historyState.jumpHistoryIndex =
      static_cast<int>(historyState.jumpHistory.size());
  historyState.jumpInputDraft =
      (jumpInput != nullptr) ? jumpInput->text() : QString();
}

bool CommandPaletteWidget::handleCommandHistoryNavigation(
    const QKeyEvent *event) {
  if (commandInput == nullptr) {
    return false;
  }

  Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier) {
    return false;
  }

  if (historyState.commandHistory.isEmpty()) {
    return false;
  }

  auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < historyState.commandHistory.size()) {
      commandInput->setText(historyState.commandHistory.at(index));
    } else {
      commandInput->setText(historyState.commandInputDraft);
    }
    commandInput->setCursorPosition(
        static_cast<int>(commandInput->text().length()));
  };

  if (event->key() == Qt::Key_Up) {
    if (historyState.commandHistoryIndex ==
        historyState.commandHistory.size()) {
      historyState.commandInputDraft = commandInput->text();
    }

    historyState.commandHistoryIndex =
        std::max(0, historyState.commandHistoryIndex - 1);
    applyHistoryEntry(historyState.commandHistoryIndex);
    historyState.currentlyInHistory = true;
    return true;
  }

  if (event->key() == Qt::Key_Down) {
    if (historyState.commandHistoryIndex ==
        historyState.commandHistory.size()) {
      historyState.commandInputDraft = commandInput->text();
    }

    historyState.commandHistoryIndex =
        std::min(historyState.commandHistoryIndex + 1,
                 static_cast<int>(historyState.commandHistory.size()));
    applyHistoryEntry(historyState.commandHistoryIndex);
    historyState.currentlyInHistory =
        historyState.commandHistoryIndex < historyState.commandHistory.size();

    if (!historyState.currentlyInHistory && (commandSuggestions != nullptr) &&
        commandSuggestions->count() > 0) {
      commandSuggestions->setCurrentRow(0);
    }

    return true;
  }

  return false;
}

void CommandPaletteWidget::saveCommandHistoryEntry(const QString &entry) {
  if (entry.isEmpty()) {
    return;
  }

  if (historyState.commandHistory.isEmpty() ||
      historyState.commandHistory.back() != entry) {
    historyState.commandHistory.append(entry);
    if (historyState.commandHistory.size() > k::COMMAND_HISTORY_LIMIT) {
      historyState.commandHistory.removeFirst();
    }
  }

  resetCommandHistoryNavigation();
}

// TODO(scarlet): Break this up into smaller fns
bool CommandPaletteWidget::handleCommandSuggestionNavigation(
    const QKeyEvent *event) {
  if ((commandInput == nullptr) || (commandSuggestions == nullptr) ||
      !commandSuggestions->isVisible() || commandSuggestions->count() == 0) {
    return false;
  }

  const Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier) {
    return false;
  }

  if (historyState.currentlyInHistory &&
      (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
    return false;
  }

  if (event->key() == Qt::Key_Up && commandSuggestions->currentRow() <= 0 &&
      !historyState.commandHistory.empty()) {
    // Clear selection and enable entering command history
    commandSuggestions->clearSelection();
    commandSuggestions->setCurrentRow(-1);
    return false;
  }

  const auto clampRow = [&](int row) {
    if (commandSuggestions->count() == 0) {
      return 0;
    }

    return std::clamp(row, 0, commandSuggestions->count() - 1);
  };

  if (event->key() == Qt::Key_Down) {
    int nextRow = commandSuggestions->currentRow();
    nextRow = nextRow < 0 ? 0 : nextRow + 1;
    commandSuggestions->setCurrentRow(clampRow(nextRow));
    return true;
  }

  if (event->key() == Qt::Key_Up) {
    int prevRow = commandSuggestions->currentRow();
    prevRow = prevRow <= 0 ? 0 : prevRow - 1;
    commandSuggestions->setCurrentRow(clampRow(prevRow));
    return true;
  }

  if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Return ||
      event->key() == Qt::Key_Enter) {
    const QListWidgetItem *current = commandSuggestions->currentItem();
    if ((current == nullptr) && commandSuggestions->count() > 0) {
      commandSuggestions->setCurrentRow(0);
      current = commandSuggestions->item(0);
    }

    if (current != nullptr) {
      commandInput->setText(current->text());
      commandInput->setCursorPosition(
          static_cast<int>(commandInput->text().length()));
      commandSuggestions->clearSelection();
      commandSuggestions->setCurrentRow(-1);
      if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emitCommandRequestFromInput();
        return true;
      }
    }

    if (event->key() == Qt::Key_Tab) {
      return true;
    }
  }

  return false;
}

void CommandPaletteWidget::updateCommandSuggestions(const QString &text) {
  const bool hasList = (commandSuggestions != nullptr);

  if (!hasList) {
    setVisibleIf(commandPaletteBottomDivider, false);
    setSpacerHeight(commandBottomSpacer, 0);
    return;
  }

  setSpacerHeight(commandBottomSpacer, k::TOP_SPACER_HEIGHT);
  setSpacerHeight(commandTopSpacer, 0);

  commandSuggestions->clear();

  const QString needle = text.trimmed();
  for (const CommandDef &def : COMMANDS) {
    if (needle.isEmpty() ||
        def.displayString.contains(needle, Qt::CaseInsensitive)) {
      commandSuggestions->addItem(def.displayString);
    }
  }

  const bool hasSuggestions = (commandSuggestions->count() > 0);

  commandSuggestions->setVisible(hasSuggestions);
  setVisibleIf(commandPaletteBottomDivider, hasSuggestions);
  setVisibleIf(commandTopDivider, hasSuggestions);

  if (!hasSuggestions) {
    commandSuggestions->setFixedHeight(0);
    setSpacerHeight(commandBottomSpacer, 0);
    setSpacerHeight(commandTopSpacer, k::TOP_SPACER_HEIGHT);
    return;
  }

  const int rowHeight = std::max(1, commandSuggestions->sizeHintForRow(0));
  commandSuggestions->setFixedHeight(rowHeight * commandSuggestions->count());
  commandSuggestions->setCurrentRow(0);
}

void CommandPaletteWidget::resetCommandHistoryNavigation() {
  historyState.commandHistoryIndex =
      static_cast<int>(historyState.commandHistory.size());
  historyState.commandInputDraft =
      (commandInput != nullptr) ? commandInput->text() : QString();
  historyState.currentlyInHistory = false;
}

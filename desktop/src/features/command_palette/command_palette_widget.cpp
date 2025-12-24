#import "command_palette_widget.h"
#include "features/command_palette/palette_divider.h"
#include "utils/gui_utils.h"

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
  setMinimumWidth(MIN_WIDTH);
  setMaximumWidth(WIDTH);
}

void CommandPaletteWidget::buildUi() {
  mainFrame =
      new PaletteFrame({theme.backgroundColor, theme.borderColor}, this);

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                                 CONTENT_MARGIN);
  rootLayout->setSizeConstraint(QLayout::SetFixedSize);
  rootLayout->addWidget(mainFrame);

  frameLayout = new QVBoxLayout(mainFrame);
  frameLayout->setSpacing(FRAME_LAYOUT_SPACING);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frameLayout->setAlignment(Qt::AlignTop);

  auto *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(SHADOW_BLUR_RADIUS);
  shadow->setColor(Qt::black);
  shadow->setOffset(SHADOW_X_OFFSET, SHADOW_Y_OFFSET);
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
    updateHistoryHint(jumpInput, HISTORY_HINT);
  });

  connect(commandInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitCommandRequestFromInput);
  connect(commandInput, &QLineEdit::textEdited, this,
          [this](const QString &) { resetCommandHistoryNavigation(); });
  connect(commandInput, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            updateHistoryHint(commandInput, HISTORY_HINT);
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

  auto font = makeInterfaceFont(JUMP_FONT_SIZE);

  auto *layout = new QVBoxLayout(jumpPage);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(FRAME_LAYOUT_SPACING);
  layout->setAlignment(Qt::AlignTop);

  // Top spacer
  layout->addItem(buildSpacer(TOP_SPACER_HEIGHT));

  // Jump input
  jumpInput = new QLineEdit(jumpPage);
  jumpInput->setFont(font);
  jumpInput->setStyleSheet(
      QString(JUMP_INPUT_STYLE).arg(theme.foregroundColor));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  jumpInput->installEventFilter(this);
  jumpInput->setMinimumWidth(
      static_cast<int>(WIDTH / JUMP_INPUT_WIDTH_DIVIDER));
  layout->addWidget(jumpInput);

  // Divider
  auto *divider = buildDivider(jumpPage, theme.borderColor);
  layout->addWidget(divider);

  // Spacer
  layout->addItem(CommandPaletteWidget::buildSpacer(LABEL_TOP_SPACER_HEIGHT));

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
      CommandPaletteWidget::buildSpacer(LABEL_BOTTOM_SPACER_HEIGHT));

  pages->addWidget(jumpPage);
}

void CommandPaletteWidget::buildCommandPage() {
  commandPage = new QWidget(pages);

  auto font = makeInterfaceFont(JUMP_FONT_SIZE);

  auto *layout = new QVBoxLayout(commandPage);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(FRAME_LAYOUT_SPACING);

  // Top spacer
  layout->addItem(buildSpacer(TOP_SPACER_HEIGHT));

  // Command input
  commandInput = new QLineEdit(commandPage);
  commandInput->setFont(font);
  const QString placeholderText = COMMAND_PLACEHOLDER_TEXT;
  commandInput->setPlaceholderText(placeholderText);
  commandInput->setStyleSheet(
      QString(JUMP_INPUT_STYLE).arg(theme.foregroundColor));
  commandInput->setClearButtonEnabled(false);
  commandInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  commandInput->installEventFilter(this);
  commandInput->setMinimumWidth(WIDTH / COMMAND_INPUT_WIDTH_DIVIDER);
  layout->addWidget(commandInput);

  // Divider
  auto *divider = buildDivider(commandPage, theme.borderColor);
  layout->addWidget(divider);

  // Suggestions
  auto *commandSuggestions = buildCommandSuggestionsList(commandPage, font);
  layout->addWidget(commandSuggestions);

  // Spacer
  layout->addSpacerItem(buildSpacer(TOP_SPACER_HEIGHT));

  pages->addWidget(commandPage);
}

void CommandPaletteWidget::adjustPosition() {
  if (parentWidget() == nullptr) {
    return;
  }

  const int availableWidth = std::max(
      0, parentWidget()->width() - static_cast<int>(CONTENT_MARGIN * 2));
  const int xPos = static_cast<int>((parentWidget()->width() - WIDTH) / 2.0);
  const int yPos = static_cast<int>(TOP_OFFSET);

  move(parentWidget()->mapToGlobal(QPoint(xPos, yPos)));
}

QWidget *CommandPaletteWidget::buildShortcutsContainer(QWidget *parent) {
  const auto font = makeInterfaceFont(JUMP_FONT_SIZE);

  struct ShortcutRow {
    const char *code;
    const char *desc;
  };

  // NOLINTNEXTLINE
  static const ShortcutRow rows[] = {
      {NAV[0].key.data(), "current line beginning"},  // NOLINT
      {NAV[1].key.data(), "current line middle"},     // NOLINT
      {NAV[2].key.data(), "current line end"},        // NOLINT
      {NAV[3].key.data(), "document beginning"},      // NOLINT
      {NAV[4].key.data(), "document middle"},         // NOLINT
      {NAV[5].key.data(), "document end"},            // NOLINT
      {NAV[6].key.data(), "document quarter"},        // NOLINT
      {NAV[7].key.data(), "document three-quarters"}, // NOLINT
      {NAV[8].key.data(), "last jumped-to position"}, // NOLINT
  };

  const QFontMetrics fontMetrics(font);
  int codeColumnWidth = 0;
  for (const auto &row : rows) {
    codeColumnWidth =
        std::max(codeColumnWidth, fontMetrics.horizontalAdvance(row.code));
  }
  codeColumnWidth += fontMetrics.horizontalAdvance("  ");

  shortcutsContainer = new QWidget(parent);
  auto *shortcutsLayout = new QVBoxLayout(shortcutsContainer);
  shortcutsLayout->setContentsMargins(4, 0, 4, 0);
  shortcutsLayout->setSpacing(2);

  const QString hintStyle =
      QString(LABEL_STYLE).arg(theme.foregroundVeryMutedColor);
  const auto createHintLabel = [&](const QString &text) {
    return UiUtils::createLabel(text, hintStyle, font, parent, false,
                                QSizePolicy::Fixed, QSizePolicy::Fixed);
  };

  for (const auto &row : rows) {
    auto *rowWidget = new QWidget(shortcutsContainer);
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN, 0,
                                  COMMAND_ROW_HORIZONTAL_CONTENT_MARGIN, 0);
    rowLayout->setSpacing(4);

    auto *codeLabel =
        UiUtils::createLabel(row.code, hintStyle, font, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);
    codeLabel->setMinimumWidth(
        static_cast<int>(codeColumnWidth / CODE_LABEL_WIDTH_DIVIDER));

    auto *dashLabel =
        UiUtils::createLabel("", hintStyle, font, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);
    dashLabel->setMinimumWidth(
        static_cast<int>(codeColumnWidth / DASH_LABEL_WIDTH_DIVIDER));
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
  QFont font = makeInterfaceFont(JUMP_FONT_SIZE);

  auto *shortcutsRow = new QWidget(jumpPage);

  auto *shortcutsRowLayout = new QHBoxLayout(shortcutsRow);
  shortcutsRowLayout->setContentsMargins(0, 2, 0, 0);
  shortcutsRowLayout->setSpacing(SHORTCUTS_ROW_SPACING);

  shortcutsToggle = new QToolButton(shortcutsRow);
  shortcutsToggle->setText(SHORTCUTS_BUTTON_TEXT);
  shortcutsToggle->setCheckable(true);
  shortcutsToggle->setChecked(showJumpShortcuts);
  shortcutsToggle->setArrowType(Qt::DownArrow);
  shortcutsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  shortcutsToggle->setFont(font);
  shortcutsToggle->setStyleSheet(
      QString(SHORTCUTS_BUTTON_STYLE)
          .arg(theme.foregroundColor, theme.foregroundVeryMutedColor));
  shortcutsToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  shortcutsRowLayout->addWidget(shortcutsToggle);

  if (shortcutsToggleShortcut != nullptr) {
    const QString shortcutText =
        shortcutsToggleShortcut->key().toString(QKeySequence::NativeText);

    if (!shortcutText.isEmpty()) {
      auto *shortcutHint = UiUtils::createLabel(
          shortcutText,
          QString(LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
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
  commandSuggestions = new QListWidget(parent);
  commandSuggestions->setFont(font);
  commandSuggestions->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  commandSuggestions->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  commandSuggestions->setFrameShape(QFrame::NoFrame);
  commandSuggestions->setSelectionMode(QAbstractItemView::SingleSelection);
  commandSuggestions->setUniformItemSizes(true);
  commandSuggestions->setVisible(false);
  commandSuggestions->setFocusPolicy(Qt::NoFocus);

  const QString suggestionStyle =
      QString(COMMAND_SUGGESTION_STYLE)
          .arg(theme.foregroundColor, theme.accentMutedColor,
               theme.accentForegroundColor);
  commandSuggestions->setStyleSheet(suggestionStyle);

  return commandSuggestions;
}

void CommandPaletteWidget::buildHistoryHint(QWidget *targetInput,
                                            const QFont &font) {
  historyHint = UiUtils::createLabel(
      "",
      QString(LABEL_STYLE).arg(theme.foregroundVeryMutedColor) +
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
      "} QFrame{ border-radius: 12px; background: %1; border: 2px "
      "solid %2; }";
  setStyleSheet(stylesheet.arg(theme.backgroundColor, theme.borderColor));

  if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(
          mainFrame->graphicsEffect())) {
    shadow->setColor(theme.shadowColor);
  }

  mainFrame->setAndApplyTheme({theme.backgroundColor, theme.borderColor});

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
      updateHistoryHint(jumpInput, HISTORY_HINT);
    } else if (obj == commandInput) {
      updateHistoryHint(commandInput, HISTORY_HINT);
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

  const auto *const foundEntry =
      std::find_if(NAV.begin(), NAV.end(),
                   [&](const NavEntry &entry) { return entry.key == value; });
  if (foundEntry != NAV.end()) {
    if (text != NAV[JUMP_TO_LAST_TARGET_INDEX].key.data()) {
      storeEntry(text);
    }

    (this->*(foundEntry->fn))();
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

  const auto lsKey = NAV[8].key;
  for (int i = static_cast<int>(historyState.jumpHistory.size()) - 1; i >= 0;
       --i) {
    const auto &entry = historyState.jumpHistory.at(i);
    if (entry.trimmed() ==
        QString::fromLatin1(lsKey.data(), static_cast<int>(lsKey.size()))) {
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
  font.setPointSizeF(LABEL_FONT_SIZE);
  const QString styleSheet =
      QString(LABEL_STYLE).arg(theme.foregroundVeryMutedColor);
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
    if (historyState.jumpHistory.size() > JUMP_HISTORY_LIMIT) {
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
    if (historyState.commandHistory.size() > COMMAND_HISTORY_LIMIT) {
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
  if (commandSuggestions == nullptr) {
    if (commandPaletteBottomDivider != nullptr) {
      commandPaletteBottomDivider->hide();
    }
    return;
  }

  commandSuggestions->clear();

  const QString command = text.trimmed();
  for (const CommandDef &commandDef : COMMANDS) {
    if (command.isEmpty() ||
        commandDef.displayString.contains(command, Qt::CaseInsensitive)) {
      commandSuggestions->addItem(commandDef.displayString);
    }
  }

  const int suggestionCount = commandSuggestions->count();
  const bool hasSuggestions = suggestionCount > 0;

  commandSuggestions->setVisible(hasSuggestions);
  if (commandPaletteBottomDivider != nullptr) {
    commandPaletteBottomDivider->setVisible(hasSuggestions);
  }

  if (!hasSuggestions) {
    commandSuggestions->setFixedHeight(0);
    return;
  }

  const int rowHeight = std::max(1, commandSuggestions->sizeHintForRow(0));
  const int desiredHeight = rowHeight * commandSuggestions->count();

  commandSuggestions->setFixedHeight(desiredHeight);
  commandSuggestions->setCurrentRow(0);
}

void CommandPaletteWidget::resetCommandHistoryNavigation() {
  historyState.commandHistoryIndex =
      static_cast<int>(historyState.commandHistory.size());
  historyState.commandInputDraft =
      (commandInput != nullptr) ? commandInput->text() : QString();
  historyState.currentlyInHistory = false;
}

#import "command_palette_widget.h"
#include "current_size_stacked_widget.h"
#include "features/command_palette/palette_divider.h"
#include "utils/gui_utils.h"

CommandPaletteWidget::CommandPaletteWidget(const CommandPaletteTheme &theme,
                                           neko::ConfigManager &configManager,
                                           QWidget *parent)
    : QWidget(parent), parent(parent), theme(theme),
      configManager(configManager) {
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

void CommandPaletteWidget::buildJumpPage() {
  jumpPage = new QWidget(pages);

  const auto colors = loadPaletteColors();
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
  jumpInput->setStyleSheet(QString(JUMP_INPUT_STYLE).arg(colors.foreground));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  jumpInput->installEventFilter(this);
  jumpInput->setMinimumWidth(
      static_cast<int>(WIDTH / JUMP_INPUT_WIDTH_DIVIDER));
  layout->addWidget(jumpInput);

  // Divider
  auto *divider = buildDivider(jumpPage, colors.border);
  layout->addWidget(divider);

  // Spacer
  layout->addItem(CommandPaletteWidget::buildSpacer(LABEL_TOP_SPACER_HEIGHT));

  // Current line label
  currentLineLabel = buildCurrentLineLabel(jumpPage, colors, font);
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

  const auto colors = loadPaletteColors();
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
  commandInput->setStyleSheet(QString(JUMP_INPUT_STYLE).arg(colors.foreground));
  commandInput->setClearButtonEnabled(false);
  commandInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  commandInput->installEventFilter(this);
  commandInput->setMinimumWidth(WIDTH / COMMAND_INPUT_WIDTH_DIVIDER);
  layout->addWidget(commandInput);

  // Divider
  auto *divider = buildDivider(commandPage, colors.border);
  layout->addWidget(divider);

  // Suggestions
  auto *commandSuggestions =
      buildCommandSuggestionsList(commandPage, colors, font);
  layout->addWidget(commandSuggestions);

  // Spacer
  layout->addSpacerItem(buildSpacer(TOP_SPACER_HEIGHT));

  pages->addWidget(commandPage);
}

void CommandPaletteWidget::updateJumpUiFromState() {
  if ((jumpInput == nullptr) || (currentLineLabel == nullptr)) {
    return;
  }

  const int row0 = currentRow;
  const int col0 = std::clamp(currentColumn, 0, std::max(0, maxColumn - 1));

  jumpInput->setPlaceholderText(QString("%1:%2").arg(row0 + 1).arg(col0 + 1));

  currentLineLabel->setText(QString("Current line: %1 of %2 (column %3)")
                                .arg(row0 + 1)
                                .arg(maxLineCount)
                                .arg(col0 + 1));
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

void CommandPaletteWidget::showPalette() {
  currentMode = Mode::None;
  pages->setCurrentWidget(commandPage);

  resetCommandHistoryNavigation();
  commandInput->setText("");

  updateCommandSuggestions(commandInput->text());
  show();

  commandInput->setFocus();
}

void CommandPaletteWidget::jumpToRowColumn(int currentRow, int currentColumn,
                                           int maxColumn, int lineCount,
                                           int lastMaxColumn) {
  prepareJumpState(currentRow, currentColumn, maxColumn, lineCount,
                   lastMaxColumn);
  updateJumpUiFromState();

  resetJumpHistoryNavigation();
  jumpInput->setText("");

  pages->setCurrentWidget(jumpPage);
  show();

  jumpInput->setFocus();
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

void CommandPaletteWidget::prepareJumpState(const int currentRow,
                                            const int currentCol,
                                            const int maxCol,
                                            const int lineCount,
                                            const int lastLineMaxCol) {
  currentMode = Mode::GoToPosition;
  maxLineCount = std::max(1, lineCount);
  maxColumn = std::max(1, maxCol);
  lastLineMaxColumn = std::max(1, lastLineMaxCol);
  maxRow = std::max(1, lineCount);
  this->currentRow = std::clamp(currentRow, 0, maxLineCount - 1);
  this->currentColumn = std::clamp(currentCol, 0, maxCol);
}

void CommandPaletteWidget::emitCommandRequestFromInput() {
  if (commandInput == nullptr) {
    return;
  }

  const QString text = commandInput->text().trimmed();
  if (text.isEmpty()) {
    close();
    return;
  }

  const auto storeEntry = [this](const QString &entry) {
    saveCommandHistoryEntry(entry);
  };

  // TODO(scarlet): Allow aliases?
  if (text == TOGGLE_FILE_EXPLORER_COMMAND || text == SET_THEME_TO_LIGHT ||
      text == SET_THEME_TO_DARK) {
    storeEntry(text);
    emit commandRequested(text);
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

  const int effectiveMaxLine = std::max(1, maxLineCount);
  row = std::clamp(row, 1, effectiveMaxLine);
  col = std::max(1, col);

  storeEntry(text);
  emit goToPositionRequested(row - 1, col - 1);
  close();
}

// TODO(scarlet): Consolidate these?
void CommandPaletteWidget::jumpToLineStart() {
  emit goToPositionRequested(currentRow, 0);
  close();
}

void CommandPaletteWidget::jumpToLineEnd() {
  emit goToPositionRequested(currentRow, maxColumn);
  close();
}

void CommandPaletteWidget::jumpToLineMiddle() {
  emit goToPositionRequested(currentRow, maxColumn / 2);
  close();
}

void CommandPaletteWidget::jumpToDocumentStart() {
  emit goToPositionRequested(0, 0);
  close();
}

void CommandPaletteWidget::jumpToDocumentMiddle() {
  emit goToPositionRequested(maxRow / 2, currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentQuarter() {
  emit goToPositionRequested(maxRow / 4, currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentThreeQuarters() {
  emit goToPositionRequested((maxRow / 4) * 3, currentColumn);
  close();
}

void CommandPaletteWidget::jumpToDocumentEnd() {
  emit goToPositionRequested(maxRow, lastLineMaxColumn);
  close();
}

void CommandPaletteWidget::jumpToLastTarget() {
  if ((jumpInput == nullptr) || jumpHistory.isEmpty()) {
    close();
    return;
  }

  const auto lsKey = NAV[8].key;
  for (int i = static_cast<int>(jumpHistory.size()) - 1; i >= 0; --i) {
    const auto &entry = jumpHistory.at(i);
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

CommandPaletteWidget::PaletteColors
CommandPaletteWidget::loadPaletteColors() const {
  // TODO(scarlet): Refactor this
  PaletteColors paletteColors{};
  paletteColors.foreground = theme.foregroundColor;
  paletteColors.foregroundVeryMuted = theme.foregroundVeryMutedColor;
  paletteColors.border = theme.borderColor;
  paletteColors.accent = theme.accentMutedColor;
  paletteColors.accentForeground = theme.accentForegroundColor;
  return paletteColors;
}

QFont CommandPaletteWidget::makeInterfaceFont(const qreal pointSize) const {
  auto font = UiUtils::loadFont(configManager, neko::FontType::Interface);
  font.setPointSizeF(pointSize);
  return font;
}

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

QListWidget *CommandPaletteWidget::buildCommandSuggestionsList(
    QWidget *parent, const PaletteColors &paletteColors, const QFont &font) {
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
          .arg(paletteColors.foreground, paletteColors.accent,
               paletteColors.accentForeground);
  commandSuggestions->setStyleSheet(suggestionStyle);

  return commandSuggestions;
}

QLabel *CommandPaletteWidget::buildCurrentLineLabel(
    QWidget *parent, const PaletteColors &paletteColors, QFont font) {
  font.setPointSizeF(LABEL_FONT_SIZE);
  const QString styleSheet =
      QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted);
  auto *label =
      UiUtils::createLabel("", styleSheet + "padding-left: 12px;", font, parent,
                           false, QSizePolicy::Fixed, QSizePolicy::Fixed);

  return label;
}

QWidget *CommandPaletteWidget::buildShortcutsRow(QWidget *parent) {
  const auto colors = loadPaletteColors();
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
          .arg(colors.foreground, colors.foregroundVeryMuted));
  shortcutsToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  shortcutsRowLayout->addWidget(shortcutsToggle);

  if (shortcutsToggleShortcut != nullptr) {
    const QString shortcutText =
        shortcutsToggleShortcut->key().toString(QKeySequence::NativeText);

    if (!shortcutText.isEmpty()) {
      auto *shortcutHint = UiUtils::createLabel(
          shortcutText,
          QString(LABEL_STYLE).arg(colors.foregroundVeryMuted) +
              QString("padding-right: 12px;"),
          font, parent, false, QSizePolicy::Fixed, QSizePolicy::Fixed);
      shortcutHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      shortcutsRowLayout->addWidget(shortcutHint);
    }
  }

  return shortcutsRow;
}

QWidget *CommandPaletteWidget::buildShortcutsContainer(QWidget *parent) {
  const auto colors = loadPaletteColors();
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
      QString(LABEL_STYLE).arg(colors.foregroundVeryMuted);
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

void CommandPaletteWidget::buildShortcutsSection(
    QLayout *parentLayout, const PaletteColors &paletteColors,
    const QFont &font) {

  parentLayout->addWidget(shortcutsContainer);
  adjustShortcutsAfterToggle(showJumpShortcuts);
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

void CommandPaletteWidget::buildHistoryHint(QWidget *targetInput,
                                            const PaletteColors &paletteColors,
                                            const QFont &font) {
  historyHint = UiUtils::createLabel(
      "",
      QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted) +
          "padding-right: 12px;",
      font, targetInput, false, QSizePolicy::Expanding, QSizePolicy::Preferred);
  historyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
  historyHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void CommandPaletteWidget::saveCommandHistoryEntry(const QString &entry) {
  if (entry.isEmpty()) {
    return;
  }

  if (commandHistory.isEmpty() || commandHistory.back() != entry) {
    commandHistory.append(entry);
    if (commandHistory.size() > COMMAND_HISTORY_LIMIT) {
      commandHistory.removeFirst();
    }
  }

  resetCommandHistoryNavigation();
}

void CommandPaletteWidget::saveJumpHistoryEntry(const QString &entry) {
  if (entry.isEmpty()) {
    return;
  }

  if (jumpHistory.isEmpty() || jumpHistory.back() != entry) {
    jumpHistory.append(entry);
    if (jumpHistory.size() > JUMP_HISTORY_LIMIT) {
      jumpHistory.removeFirst();
    }
  }

  resetJumpHistoryNavigation();
}

void CommandPaletteWidget::resetJumpHistoryNavigation() {
  jumpHistoryIndex = static_cast<int>(jumpHistory.size());
  jumpInputDraft = (jumpInput != nullptr) ? jumpInput->text() : QString();
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

  if (jumpHistory.isEmpty()) {
    return false;
  }

  const auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < jumpHistory.size()) {
      jumpInput->setText(jumpHistory.at(index));
    } else {
      jumpInput->setText(jumpInputDraft);
    }
    jumpInput->setCursorPosition(static_cast<int>(jumpInput->text().length()));
  };

  if (event->key() == Qt::Key_Up) {
    if (jumpHistoryIndex == jumpHistory.size()) {
      jumpInputDraft = jumpInput->text();
    }

    jumpHistoryIndex = std::max(0, jumpHistoryIndex - 1);
    applyHistoryEntry(jumpHistoryIndex);
    return true;
  }

  if (event->key() == Qt::Key_Down) {
    if (jumpHistoryIndex == jumpHistory.size()) {
      jumpInputDraft = jumpInput->text();
    }

    jumpHistoryIndex =
        std::min(jumpHistoryIndex + 1, static_cast<int>(jumpHistory.size()));
    applyHistoryEntry(jumpHistoryIndex);
    return true;
  }

  return false;
}

void CommandPaletteWidget::resetCommandHistoryNavigation() {
  commandHistoryIndex = static_cast<int>(commandHistory.size());
  commandInputDraft =
      (commandInput != nullptr) ? commandInput->text() : QString();
  currentlyInHistory = false;
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

  if (commandHistory.isEmpty()) {
    return false;
  }

  auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < commandHistory.size()) {
      commandInput->setText(commandHistory.at(index));
    } else {
      commandInput->setText(commandInputDraft);
    }
    commandInput->setCursorPosition(
        static_cast<int>(commandInput->text().length()));
  };

  if (event->key() == Qt::Key_Up) {
    if (commandHistoryIndex == commandHistory.size()) {
      commandInputDraft = commandInput->text();
    }

    commandHistoryIndex = std::max(0, commandHistoryIndex - 1);
    applyHistoryEntry(commandHistoryIndex);
    currentlyInHistory = true;
    return true;
  }

  if (event->key() == Qt::Key_Down) {
    if (commandHistoryIndex == commandHistory.size()) {
      commandInputDraft = commandInput->text();
    }

    commandHistoryIndex = std::min(commandHistoryIndex + 1,
                                   static_cast<int>(commandHistory.size()));
    applyHistoryEntry(commandHistoryIndex);
    currentlyInHistory = commandHistoryIndex < commandHistory.size();

    if (!currentlyInHistory && (commandSuggestions != nullptr) &&
        commandSuggestions->count() > 0) {
      commandSuggestions->setCurrentRow(0);
    }

    return true;
  }

  return false;
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

  if (currentlyInHistory &&
      (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
    return false;
  }

  if (event->key() == Qt::Key_Up && commandSuggestions->currentRow() <= 0 &&
      !commandHistory.empty()) {
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
  const QString trimmed = text.trimmed();

  for (const auto &command : AVAILABLE_COMMANDS) {
    if (trimmed.isEmpty() || command.contains(trimmed, Qt::CaseInsensitive)) {
      commandSuggestions->addItem(command);
    }
  }

  const bool hasSuggestions = commandSuggestions->count() > 0;

  if (!hasSuggestions) {
    commandSuggestions->setVisible(false);
    commandSuggestions->setFixedHeight(0);
    if (commandPaletteBottomDivider != nullptr) {
      commandPaletteBottomDivider->hide();
    }
    return;
  }

  if (commandPaletteBottomDivider != nullptr) {
    commandPaletteBottomDivider->show();
  }
  commandSuggestions->setVisible(true);

  const int rowHeight = std::max(1, commandSuggestions->sizeHintForRow(0));
  const int desiredHeight = rowHeight * commandSuggestions->count();

  commandSuggestions->setFixedHeight(desiredHeight);
  commandSuggestions->setCurrentRow(0);
}

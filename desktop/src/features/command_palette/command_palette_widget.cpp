#import "command_palette_widget.h"

CommandPaletteWidget::CommandPaletteWidget(neko::ThemeManager &themeManager,
                                           neko::ConfigManager &configManager,
                                           QWidget *parent)
    : QWidget(parent), parent(parent), themeManager(themeManager),
      configManager(configManager) {
  setWindowFlags(Qt::Popup | Qt::FramelessWindowHint |
                 Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAutoFillBackground(false);
  installEventFilter(this);
  setMinimumWidth(MIN_WIDTH);
  setMaximumWidth(WIDTH);

  mainFrame = new PaletteFrame(themeManager, this);

  QVBoxLayout *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                                 CONTENT_MARGIN);
  rootLayout->setSizeConstraint(QLayout::SetFixedSize);
  rootLayout->addWidget(mainFrame);

  frameLayout = new QVBoxLayout(mainFrame);
  frameLayout->setSpacing(8);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frameLayout->setAlignment(Qt::AlignTop);

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(SHADOW_BLUR_RADIUS);
  shadow->setColor(Qt::black);
  shadow->setOffset(SHADOW_X_OFFSET, SHADOW_Y_OFFSET);

  mainFrame->setGraphicsEffect(shadow);

  shortcutsToggleShortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
  shortcutsToggleShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  connect(shortcutsToggleShortcut, &QShortcut::activated, this, [this]() {
    if (shortcutsToggle) {
      shortcutsToggle->toggle();
    }
  });

  applyTheme();
}

CommandPaletteWidget::~CommandPaletteWidget() {}

void CommandPaletteWidget::applyTheme() {
  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "command_palette.background");
  auto borderColor =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  auto shadowColor =
      UiUtils::getThemeColor(themeManager, "command_palette.shadow");

  QString stylesheet =
      "CommandPaletteWidget { background: transparent; border: none; "
      "} QFrame{ border-radius: 12px; background: %1; border: 2px "
      "solid %2; }";
  setStyleSheet(stylesheet.arg(backgroundColor, borderColor));

  if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(
          mainFrame->graphicsEffect())) {
    shadow->setColor(shadowColor);
  }

  if (isVisible()) {
    if (currentMode == Mode::GoToPosition) {
      buildJumpContent(currentRow, currentColumn, maxColumn, maxLineCount,
                       lastLineMaxColumn);
    } else {
      buildCommandPalette();
    }
  }
}

void CommandPaletteWidget::showEvent(QShowEvent *event) {
  if (!parentWidget())
    return;

  adjustPosition();
  QWidget::showEvent(event);
}

void CommandPaletteWidget::adjustPosition() {
  if (!parentWidget())
    return;

  const int availableWidth = std::max(
      0, parentWidget()->width() - static_cast<int>(CONTENT_MARGIN * 2));
  setFixedWidth(WIDTH);

  const int x = (parentWidget()->width() - WIDTH) / 2;
  const int y = TOP_OFFSET;

  move(parentWidget()->mapToGlobal(QPoint(x, y)));
}

void CommandPaletteWidget::showPalette() {
  buildCommandPalette();
  show();

  if (commandInput) {
    commandInput->setFocus();
  }
}

void CommandPaletteWidget::jumpToRowColumn(int currentRow, int currentCol,
                                           int maxCol, int lineCount,
                                           int lastLineMaxCol) {
  buildJumpContent(currentRow, currentCol, maxCol, lineCount, lastLineMaxCol);
  show();

  if (jumpInput) {
    jumpInput->setFocus();
  }
}

void CommandPaletteWidget::jumpToDocumentStart() {
  emit goToPositionRequested(0, 0);
  close();
}

void CommandPaletteWidget::jumpToDocumentEnd() {
  emit goToPositionRequested(maxRow, lastLineMaxColumn);
  close();
}

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

void CommandPaletteWidget::jumpToLastTarget() {
  if (!jumpInput || jumpHistory.isEmpty()) {
    close();
    return;
  }

  const auto lsKey = NAV[8].key;
  for (int i = jumpHistory.size() - 1; i >= 0; --i) {
    const auto &entry = jumpHistory.at(i);
    if (entry.trimmed() ==
        QString::fromLatin1(lsKey.data(), static_cast<int>(lsKey.size())))
      continue;

    jumpInput->setText(entry);
    emitJumpRequestFromInput();
    return;
  }

  close();
}

void CommandPaletteWidget::emitCommandRequestFromInput() {
  if (!commandInput)
    return;

  const QString text = commandInput->text().trimmed();
  if (text.isEmpty()) {
    close();
    return;
  }

  auto storeEntry = [this](const QString &entry) {
    saveCommandHistoryEntry(entry);
  };

  // TODO: Allow aliases?
  if (text == TOGGLE_FILE_EXPLORER_COMMAND) {
    storeEntry(text);
    emit commandRequested(text);
  } else if (text == SET_THEME_TO_LIGHT) {
    storeEntry(text);
    emit commandRequested(text);
  } else if (text == SET_THEME_TO_DARK) {
    storeEntry(text);
    emit commandRequested(text);
  }

  close();
}

void CommandPaletteWidget::emitJumpRequestFromInput() {
  if (!jumpInput)
    return;

  const QString text = jumpInput->text().trimmed();
  if (text.isEmpty()) {
    close();
    return;
  }

  const QStringList parts = text.split(':', Qt::SkipEmptyParts);
  bool okRow = false;
  bool okCol = true;

  auto storeEntry = [this](const QString &entry) {
    saveJumpHistoryEntry(entry);
  };

  // TODO: Allow aliases?
  QString value = text;

  auto it = std::find_if(NAV.begin(), NAV.end(),
                         [&](const NavEntry &e) { return e.key == value; });
  if (it != NAV.end()) {
    if (text != NAV[8].key.data()) {
      storeEntry(text);
    }

    (this->*(it->fn))();
    return;
  }

  int row = parts.value(0).toInt(&okRow);
  int col = parts.size() > 1 ? parts.value(1).toInt(&okCol) : 1;

  if (!okRow || !okCol || row < 0 || col < 0) {
    close();
    return;
  }

  int effectiveMaxLine = std::max(1, maxLineCount);
  row = std::clamp(row, 1, effectiveMaxLine);
  col = std::max(1, col);

  storeEntry(text);
  emit goToPositionRequested(row - 1, col - 1);
  close();
}

void CommandPaletteWidget::clearContent() {
  if (!frameLayout)
    return;

  while (frameLayout->count()) {
    QLayoutItem *item = frameLayout->takeAt(0);
    if (QWidget *w = item->widget()) {
      w->deleteLater();
    }

    delete item;
  }

  commandInput = nullptr;
  jumpInput = nullptr;
  historyHint = nullptr;
  shortcutsContainer = nullptr;
  shortcutsToggle = nullptr;
  commandSuggestions = nullptr;
  commandPaletteBottomDivider = nullptr;
  currentMode = Mode::None;
}

void CommandPaletteWidget::prepareJumpState(int currentRow, int currentCol,
                                            int maxCol, int lineCount,
                                            int lastLineMaxCol) {
  currentMode = Mode::GoToPosition;
  maxLineCount = std::max(1, lineCount);
  maxColumn = std::max(1, maxCol);
  lastLineMaxColumn = std::max(1, lastLineMaxCol);
  maxRow = std::max(1, lineCount);
  this->currentRow = std::clamp(currentRow, 0, maxLineCount - 1);
  this->currentColumn = std::clamp(currentCol, 0, maxCol);
}

CommandPaletteWidget::PaletteColors
CommandPaletteWidget::loadPaletteColors() const {
  PaletteColors paletteColors{};
  paletteColors.foreground =
      UiUtils::getThemeColor(themeManager, "ui.foreground");
  paletteColors.foregroundVeryMuted =
      UiUtils::getThemeColor(themeManager, "ui.foreground.very_muted");
  paletteColors.border =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  paletteColors.accent =
      UiUtils::getThemeColor(themeManager, "ui.accent.muted");
  paletteColors.accentForeground =
      UiUtils::getThemeColor(themeManager, "ui.accent.foreground");
  return paletteColors;
}

QFont CommandPaletteWidget::makeInterfaceFont(qreal pointSize) const {
  auto font = UiUtils::loadFont(configManager, neko::FontType::Interface);
  font.setPointSizeF(pointSize);
  return font;
}

void CommandPaletteWidget::addSpacer(int height) {
  if (!frameLayout)
    return;

  frameLayout->addItem(
      new QSpacerItem(0, height, QSizePolicy::Minimum, QSizePolicy::Fixed));
}

PaletteDivider *CommandPaletteWidget::addDivider(const QString &borderColor) {
  auto *divider = new PaletteDivider(borderColor, mainFrame);
  divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  divider->setFixedHeight(1);
  divider->setStyleSheet(QString("background-color: %1;").arg(borderColor));
  frameLayout->addWidget(divider);

  return divider;
}

void CommandPaletteWidget::addCommandInputRow(
    const PaletteColors &paletteColors, QFont &font) {
  font.setPointSizeF(JUMP_FONT_SIZE);

  commandInput = new QLineEdit(mainFrame);
  commandInput->setFont(font);
  const QString placeholderText = COMMAND_PLACEHOLDER_TEXT;
  commandInput->setPlaceholderText(placeholderText);
  commandInput->setStyleSheet(
      QString(JUMP_INPUT_STYLE).arg(paletteColors.foreground));
  commandInput->setClearButtonEnabled(false);
  commandInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  commandInput->installEventFilter(this);
  commandInput->setMinimumWidth(WIDTH / 1.25);

  connect(commandInput, &QLineEdit::textEdited, this,
          [this](const QString &) { resetCommandHistoryNavigation(); });
  connect(commandInput, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            updateHistoryHint(commandInput, HISTORY_HINT);
            updateCommandSuggestions(text);
          });

  createHistoryHint(commandInput, paletteColors, font);
  updateHistoryHint(commandInput, HISTORY_HINT);

  frameLayout->addWidget(commandInput);
  commandPaletteBottomDivider = addDivider(paletteColors.border);
}

void CommandPaletteWidget::addCommandSuggestionsList(
    const PaletteColors &paletteColors, const QFont &font) {
  commandSuggestions = new QListWidget(mainFrame);
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

  connect(commandSuggestions, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            if (!item || !commandInput)
              return;

            commandInput->setText(item->text());
            commandInput->setCursorPosition(commandInput->text().length());
            emitCommandRequestFromInput();
          });

  frameLayout->addWidget(commandSuggestions);
}

void CommandPaletteWidget::addJumpInputRow(int clampedRow, int clampedCol,
                                           const PaletteColors &paletteColors,
                                           QFont &font) {
  font.setPointSizeF(JUMP_FONT_SIZE);

  jumpInput = new QLineEdit(mainFrame);
  jumpInput->setFont(font);
  const QString displayPos =
      QString("%1:%2").arg(clampedRow + 1).arg(clampedCol + 1);
  jumpInput->setPlaceholderText(displayPos);
  jumpInput->setStyleSheet(
      QString(JUMP_INPUT_STYLE).arg(paletteColors.foreground));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  jumpInput->installEventFilter(this);
  jumpInput->setMinimumWidth(WIDTH / 1.5);

  connect(jumpInput, &QLineEdit::textEdited, this,
          [this](const QString &) { resetJumpHistoryNavigation(); });
  connect(jumpInput, &QLineEdit::textChanged, this, [this](const QString &) {
    updateHistoryHint(jumpInput, HISTORY_HINT);
  });

  createHistoryHint(jumpInput, paletteColors, font);
  updateHistoryHint(jumpInput, HISTORY_HINT);

  frameLayout->addWidget(jumpInput);
}

void CommandPaletteWidget::createHistoryHint(QWidget *targetInput,
                                             const PaletteColors &paletteColors,
                                             QFont &font) {
  historyHint = UiUtils::createLabel(
      "",
      QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted) +
          "padding-right: 12px;",
      font, targetInput, false, QSizePolicy::Expanding, QSizePolicy::Preferred);
  historyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
  historyHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void CommandPaletteWidget::addCurrentLineLabel(
    int clampedRow, int clampedCol, const PaletteColors &paletteColors,
    QFont font) {
  font.setPointSizeF(LABEL_FONT_SIZE);
  const QString styleSheet = QString(LABEL_STYLE).arg(paletteColors.foreground);
  auto *label =
      UiUtils::createLabel(QString("Current line: %1 of %2 (column %3)")
                               .arg(clampedRow + 1)
                               .arg(maxLineCount)
                               .arg(clampedCol + 1),
                           styleSheet + "padding-left: 12px;", font, mainFrame,
                           false, QSizePolicy::Fixed, QSizePolicy::Fixed);
  frameLayout->addWidget(label);
}

void CommandPaletteWidget::addShortcutsSection(
    const PaletteColors &paletteColors, const QFont &font) {
  QFont shortcutFont = font;

  struct ShortcutRow {
    const char *code;
    const char *desc;
  };

  static const ShortcutRow rows[] = {
      {NAV[0].key.data(), "current line beginning"},
      {NAV[1].key.data(), "current line middle"},
      {NAV[2].key.data(), "current line end"},
      {NAV[3].key.data(), "document beginning"},
      {NAV[4].key.data(), "document middle"},
      {NAV[5].key.data(), "document end"},
      {NAV[6].key.data(), "document quarter"},
      {NAV[7].key.data(), "document three-quarters"},
      {NAV[8].key.data(), "last jumped-to position"},
  };

  QFontMetrics metrics(shortcutFont);
  int codeColWidth = 0;
  for (const auto &row : rows) {
    codeColWidth = std::max(codeColWidth, metrics.horizontalAdvance(row.code));
  }
  codeColWidth += metrics.horizontalAdvance("  ");

  QWidget *shortcutsRow = new QWidget(mainFrame);
  auto *shortcutsRowLayout = new QHBoxLayout(shortcutsRow);
  shortcutsRowLayout->setContentsMargins(0, 2, 0, 0);
  shortcutsRowLayout->setSpacing(6);

  shortcutsToggle = new QToolButton(mainFrame);
  shortcutsToggle->setText(SHORTCUTS_BUTTON_TEXT);
  shortcutsToggle->setCheckable(true);
  shortcutsToggle->setChecked(showJumpShortcuts);
  shortcutsToggle->setArrowType(Qt::DownArrow);
  shortcutsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  shortcutsToggle->setFont(shortcutFont);
  shortcutsToggle->setStyleSheet(
      QString(SHORTCUTS_BUTTON_STYLE)
          .arg(paletteColors.foreground, paletteColors.foregroundVeryMuted));
  shortcutsToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  shortcutsRowLayout->addWidget(shortcutsToggle);

  if (shortcutsToggleShortcut) {
    const QString shortcutText =
        shortcutsToggleShortcut->key().toString(QKeySequence::NativeText);
    if (!shortcutText.isEmpty()) {
      auto *shortcutHint = UiUtils::createLabel(
          shortcutText,
          QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted) +
              QString("padding-right: 12px;"),
          shortcutFont, mainFrame, false, QSizePolicy::Fixed,
          QSizePolicy::Fixed);
      shortcutHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      shortcutsRowLayout->addWidget(shortcutHint);
    }
  }
  frameLayout->addWidget(shortcutsRow);

  shortcutsContainer = new QWidget(mainFrame);
  auto *shortcutsLayout = new QVBoxLayout(shortcutsContainer);
  shortcutsLayout->setContentsMargins(4, 0, 4, 0);
  shortcutsLayout->setSpacing(2);

  const QString hintStyle =
      QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted);
  auto createHintLabel = [&](const QString &text) {
    return UiUtils::createLabel(text, hintStyle, shortcutFont, mainFrame, false,
                                QSizePolicy::Fixed, QSizePolicy::Fixed);
  };

  for (const auto &row : rows) {
    auto *rowWidget = new QWidget(shortcutsContainer);
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(16, 0, 16, 0);
    rowLayout->setSpacing(4);

    auto *codeLabel =
        UiUtils::createLabel(row.code, hintStyle, shortcutFont, rowWidget,
                             false, QSizePolicy::Fixed, QSizePolicy::Fixed);
    codeLabel->setMinimumWidth(codeColWidth / 1.5);

    auto *dashLabel =
        UiUtils::createLabel("", hintStyle, shortcutFont, rowWidget, false,
                             QSizePolicy::Fixed, QSizePolicy::Fixed);
    dashLabel->setMinimumWidth(codeColWidth / 1.93);
    auto *descLabel =
        UiUtils::createLabel(row.desc, hintStyle, shortcutFont, rowWidget,
                             false, QSizePolicy::Fixed, QSizePolicy::Fixed);

    rowLayout->addWidget(codeLabel);
    rowLayout->addWidget(dashLabel);
    rowLayout->addWidget(descLabel);
    rowLayout->addStretch(1);
    shortcutsLayout->addWidget(rowWidget);
  }

  frameLayout->addWidget(shortcutsContainer);
  adjustShortcutsAfterToggle(showJumpShortcuts);
}

void CommandPaletteWidget::buildCommandPalette() {
  clearContent();

  const auto paletteColors = loadPaletteColors();
  QFont baseFont = makeInterfaceFont(JUMP_FONT_SIZE);

  addSpacer(TOP_SPACER_HEIGHT);
  addCommandInputRow(paletteColors, baseFont);
  addCommandSuggestionsList(paletteColors, baseFont);
  addSpacer(TOP_SPACER_HEIGHT);

  connect(commandInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitCommandRequestFromInput);
  resetCommandHistoryNavigation();
  updateCommandSuggestions(commandInput->text());
  adjustSize();
  adjustPosition();
}

void CommandPaletteWidget::buildJumpContent(int currentRow, int currentCol,
                                            int maxCol, int lineCount,
                                            int lastLineMaxCol) {
  clearContent();

  prepareJumpState(currentRow, currentCol, maxCol, lineCount, lastLineMaxCol);

  const auto paletteColors = loadPaletteColors();
  QFont baseFont = makeInterfaceFont(JUMP_FONT_SIZE);

  const int clampedRow = this->currentRow;
  const int clampedCol = std::clamp(currentCol, 0, maxColumn - 1);

  addSpacer(TOP_SPACER_HEIGHT);
  addJumpInputRow(clampedRow, clampedCol, paletteColors, baseFont);
  addDivider(paletteColors.border);
  addSpacer(LABEL_TOP_SPACER_HEIGHT);
  addCurrentLineLabel(clampedRow, clampedCol, paletteColors, baseFont);
  addShortcutsSection(paletteColors, baseFont);
  addSpacer(LABEL_BOTTOM_SPACER_HEIGHT);

  connect(shortcutsToggle, &QToolButton::toggled, this,
          [this](bool checked) { adjustShortcutsAfterToggle(checked); });

  connect(jumpInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitJumpRequestFromInput);
  resetJumpHistoryNavigation();
  adjustSize();
  adjustPosition();
}

void CommandPaletteWidget::adjustShortcutsAfterToggle(bool checked) {
  if (!shortcutsContainer || !shortcutsToggle)
    return;

  shortcutsContainer->setVisible(checked);
  shortcutsToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
  shortcutsContainer->adjustSize();
  shortcutsToggle->updateGeometry();
  shortcutsContainer->updateGeometry();
  showJumpShortcuts = checked;
  this->adjustSize();
}

void CommandPaletteWidget::updateCommandSuggestions(const QString &text) {
  if (!commandSuggestions) {
    if (commandPaletteBottomDivider)
      commandPaletteBottomDivider->hide();
    return;
  }

  commandSuggestions->clear();
  const QString trimmed = text.trimmed();

  for (const auto &command : AVAILABLE_COMMANDS) {
    if (trimmed.isEmpty() || command.contains(trimmed, Qt::CaseInsensitive)) {
      commandSuggestions->addItem(command);
    }
  }

  bool hasSuggestions = commandSuggestions->count() > 0;

  if (!hasSuggestions) {
    commandSuggestions->setVisible(false);
    commandSuggestions->setFixedHeight(0);
    if (commandPaletteBottomDivider)
      commandPaletteBottomDivider->hide();
    return;
  }

  if (commandPaletteBottomDivider)
    commandPaletteBottomDivider->show();
  commandSuggestions->setVisible(true);

  const int rowHeight = std::max(1, commandSuggestions->sizeHintForRow(0));
  int desiredHeight = rowHeight * commandSuggestions->count();

  commandSuggestions->setFixedHeight(desiredHeight);
  commandSuggestions->setCurrentRow(0);
}

void CommandPaletteWidget::updateHistoryHint(QWidget *targetInput,
                                             const QString &placeholder) {
  if (!targetInput || !historyHint)
    return;

  historyHint->setText(placeholder);
  bool shouldShow = static_cast<QLineEdit *>(targetInput)->text().isEmpty();
  historyHint->setVisible(shouldShow);
  historyHint->setGeometry(targetInput->rect().adjusted(0, 0, 0, 0));
}

void CommandPaletteWidget::saveCommandHistoryEntry(const QString &entry) {
  if (entry.isEmpty())
    return;

  if (commandHistory.isEmpty() || commandHistory.back() != entry) {
    commandHistory.append(entry);
    if (commandHistory.size() > COMMAND_HISTORY_LIMIT) {
      commandHistory.removeFirst();
    }
  }

  resetCommandHistoryNavigation();
}

void CommandPaletteWidget::saveJumpHistoryEntry(const QString &entry) {
  if (entry.isEmpty())
    return;

  if (jumpHistory.isEmpty() || jumpHistory.back() != entry) {
    jumpHistory.append(entry);
    if (jumpHistory.size() > JUMP_HISTORY_LIMIT) {
      jumpHistory.removeFirst();
    }
  }

  resetJumpHistoryNavigation();
}

void CommandPaletteWidget::resetCommandHistoryNavigation() {
  commandHistoryIndex = commandHistory.size();
  commandInputDraft = commandInput ? commandInput->text() : QString();
  currentlyInHistory = false;
}

bool CommandPaletteWidget::handleCommandSuggestionNavigation(QKeyEvent *event) {
  if (!commandInput || !commandSuggestions ||
      !commandSuggestions->isVisible() || commandSuggestions->count() == 0)
    return false;

  Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier)
    return false;

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

  auto clampRow = [&](int row) {
    if (commandSuggestions->count() == 0)
      return 0;

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
    QListWidgetItem *current = commandSuggestions->currentItem();
    if (!current && commandSuggestions->count() > 0) {
      commandSuggestions->setCurrentRow(0);
      current = commandSuggestions->item(0);
    }

    if (current) {
      commandInput->setText(current->text());
      commandInput->setCursorPosition(commandInput->text().length());
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

bool CommandPaletteWidget::handleCommandHistoryNavigation(QKeyEvent *event) {
  if (!commandInput)
    return false;

  Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier)
    return false;

  if (commandHistory.isEmpty())
    return false;

  auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < commandHistory.size()) {
      commandInput->setText(commandHistory.at(index));
    } else {
      commandInput->setText(commandInputDraft);
    }
    commandInput->setCursorPosition(commandInput->text().length());
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

    commandHistoryIndex =
        std::min(commandHistoryIndex + 1, (int)commandHistory.size());
    applyHistoryEntry(commandHistoryIndex);
    currentlyInHistory = commandHistoryIndex < commandHistory.size();

    if (!currentlyInHistory && commandSuggestions &&
        commandSuggestions->count() > 0) {
      commandSuggestions->setCurrentRow(0);
    }

    return true;
  }

  return false;
}

void CommandPaletteWidget::resetJumpHistoryNavigation() {
  jumpHistoryIndex = jumpHistory.size();
  jumpInputDraft = jumpInput ? jumpInput->text() : QString();
}

bool CommandPaletteWidget::handleJumpHistoryNavigation(QKeyEvent *event) {
  if (!jumpInput)
    return false;

  Qt::KeyboardModifiers mods =
      event->modifiers() &
      static_cast<Qt::KeyboardModifier>(~Qt::KeypadModifier);
  if (mods != Qt::NoModifier)
    return false;

  if (jumpHistory.isEmpty())
    return false;

  auto applyHistoryEntry = [this](int index) {
    if (index >= 0 && index < jumpHistory.size()) {
      jumpInput->setText(jumpHistory.at(index));
    } else {
      jumpInput->setText(jumpInputDraft);
    }
    jumpInput->setCursorPosition(jumpInput->text().length());
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

    jumpHistoryIndex = std::min(jumpHistoryIndex + 1, (int)jumpHistory.size());
    applyHistoryEntry(jumpHistoryIndex);
    return true;
  }

  return false;
}

bool CommandPaletteWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == jumpInput && event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);

    if (handleJumpHistoryNavigation(keyEvent)) {
      return true;
    }
  }

  if (obj == commandInput && event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);

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
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    if (!this->rect().contains(
            this->mapFromGlobal(mouseEvent->globalPosition().toPoint()))) {
      this->close();
      return true;
    }
  }

  return QWidget::eventFilter(obj, event);
}

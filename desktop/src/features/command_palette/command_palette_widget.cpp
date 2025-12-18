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
  shadow->setColor(shadowColor);
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
}

CommandPaletteWidget::~CommandPaletteWidget() {}

void CommandPaletteWidget::showEvent(QShowEvent *event) {
  if (!parentWidget())
    return;

  int x = (parentWidget()->width() - WIDTH) / 2;
  int y = TOP_OFFSET;

  move(parentWidget()->mapToGlobal(QPoint(x, y)));

  QWidget::showEvent(event);
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

  for (int i = jumpHistory.size() - 1; i >= 0; --i) {
    const auto &entry = jumpHistory.at(i);
    if (entry == LAST_TARGET_SHORTCUT)
      continue;

    jumpInput->setText(entry);
    emitJumpRequestFromInput();
    return;
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
  if (text == LINE_END_SHORTCUT) {
    storeEntry(text);
    jumpToLineEnd();
    return;
  } else if (text == LINE_START_SHORTCUT) {
    storeEntry(text);
    jumpToLineStart();
    return;
  } else if (text == LINE_MIDDLE_SHORTCUT) {
    storeEntry(text);
    jumpToLineMiddle();
    return;
  } else if (text == DOCUMENT_END_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentEnd();
    return;
  } else if (text == DOCUMENT_START_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentStart();
    return;
  } else if (text == DOCUMENT_MIDDLE_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentMiddle();
    return;
  } else if (text == DOCUMENT_QUARTER_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentQuarter();
    return;
  } else if (text == DOCUMENT_THREE_QUARTERS_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentThreeQuarters();
    return;
  } else if (text == LAST_TARGET_SHORTCUT) {
    jumpToLastTarget();
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

  jumpInput = nullptr;
  historyHint = nullptr;
  shortcutsContainer = nullptr;
  shortcutsToggle = nullptr;
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

void CommandPaletteWidget::addDivider(const QString &borderColor) {
  auto *divider = new PaletteDivider(borderColor, mainFrame);
  divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  divider->setFixedWidth(WIDTH / 1.5);
  divider->setFixedHeight(1);
  divider->setStyleSheet(QString("background-color: %1;").arg(borderColor));
  frameLayout->addWidget(divider);
}

void CommandPaletteWidget::addCommandInputRow(
    const PaletteColors &paletteColors, QFont &font) {
  font.setPointSizeF(JUMP_FONT_SIZE);

  commandInput = new QLineEdit(mainFrame);
  commandInput->setFont(font);
  const QString placeholderText = "Execute a command";
  commandInput->setPlaceholderText(placeholderText);
  commandInput->setStyleSheet(
      QString(JUMP_INPUT_STYLE).arg(paletteColors.foreground));
  commandInput->setClearButtonEnabled(false);
  commandInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  commandInput->installEventFilter(this);
  commandInput->setMinimumWidth(WIDTH / 1.25);

  connect(commandInput, &QLineEdit::textEdited, this,
          [this](const QString &) {});
  connect(commandInput, &QLineEdit::textChanged, this, [this](const QString &) {
    updateHistoryHint(commandInput, HISTORY_HINT);
  });

  createHistoryHint(commandInput, paletteColors, font);
  updateHistoryHint(commandInput, HISTORY_HINT);

  frameLayout->addWidget(commandInput);
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
      {LINE_END_SHORTCUT, "current line end"},
      {LINE_MIDDLE_SHORTCUT, "current line middle"},
      {LINE_START_SHORTCUT, "current line beginning"},
      {DOCUMENT_END_SHORTCUT, "document end"},
      {DOCUMENT_START_SHORTCUT, "document beginning"},
      {DOCUMENT_MIDDLE_SHORTCUT, "document middle"},
      {DOCUMENT_QUARTER_SHORTCUT, "document quarter"},
      {DOCUMENT_THREE_QUARTERS_SHORTCUT, "document three-quarters"},
      {LAST_TARGET_SHORTCUT, "last jumped-to position"},
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
  addSpacer(LABEL_TOP_SPACER_HEIGHT);

  adjustSize();
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

void CommandPaletteWidget::updateHistoryHint(QWidget *targetInput,
                                             const QString &placeholder) {
  if (!targetInput || !historyHint)
    return;

  historyHint->setText(placeholder);
  bool shouldShow = static_cast<QLineEdit *>(targetInput)->text().isEmpty();
  historyHint->setVisible(shouldShow);
  historyHint->setGeometry(targetInput->rect().adjusted(0, 0, 0, 0));
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

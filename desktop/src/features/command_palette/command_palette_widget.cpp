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

void CommandPaletteWidget::jumpToRowColumn(int currentRow, int currentCol,
                                           int maxCol, int lineCount) {
  buildJumpContent(currentRow, currentCol, maxCol, lineCount);
  show();

  if (jumpInput) {
    jumpInput->setFocus();
  }
}

void CommandPaletteWidget::jumpToDocumentStart() {
  emit goToPositionRequested(0, 0);
}

void CommandPaletteWidget::jumpToDocumentEnd() {
  emit goToPositionRequested(maxRow, maxColumn);
}

void CommandPaletteWidget::jumpToLineStart() {
  emit goToPositionRequested(currentRow, 0);
  close();
}

void CommandPaletteWidget::jumpToLineEnd() {
  emit goToPositionRequested(currentRow, maxColumn);
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
  } else if (text == DOCUMENT_END_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentEnd();
    return;
  } else if (text == DOCUMENT_START_SHORTCUT) {
    storeEntry(text);
    jumpToDocumentStart();
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
  jumpPlaceholderHint = nullptr;
  shortcutsContainer = nullptr;
  shortcutsToggle = nullptr;
  currentMode = Mode::None;
}

void CommandPaletteWidget::prepareJumpState(int currentRow, int currentCol,
                                            int maxCol, int lineCount) {
  currentMode = Mode::GoToPosition;
  maxLineCount = std::max(1, lineCount);
  maxColumn = std::max(1, maxCol);
  maxRow = std::max(1, lineCount);
  this->currentRow = std::clamp(currentRow, 0, maxLineCount - 1);
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
    updateJumpPlaceholderHint(JUMP_PLACEHOLDER_HINT);
  });

  jumpPlaceholderHint = UiUtils::createLabel(
      "", QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted), font,
      jumpInput, false, QSizePolicy::Expanding, QSizePolicy::Preferred);
  jumpPlaceholderHint->setAttribute(Qt::WA_TransparentForMouseEvents);
  jumpPlaceholderHint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  updateJumpPlaceholderHint(JUMP_PLACEHOLDER_HINT);

  frameLayout->addWidget(jumpInput);
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
                           styleSheet, font, mainFrame, false,
                           QSizePolicy::Fixed, QSizePolicy::Fixed);
  frameLayout->addWidget(label);
}

void CommandPaletteWidget::addShortcutsSection(
    const PaletteColors &paletteColors, const QFont &font) {
  QFont shortcutFont = font;

  QWidget *shortcutsRow = new QWidget(mainFrame);
  auto *shortcutsRowLayout = new QHBoxLayout(shortcutsRow);
  shortcutsRowLayout->setContentsMargins(0, 0, 0, 0);
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
          QString(LABEL_STYLE).arg(paletteColors.foregroundVeryMuted),
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

  shortcutsLayout->addWidget(createHintLabel("le - current line end"));
  shortcutsLayout->addWidget(createHintLabel("lb - current line beginning"));
  shortcutsLayout->addWidget(createHintLabel("de - document end"));
  shortcutsLayout->addWidget(createHintLabel("db - document beginning"));

  frameLayout->addWidget(shortcutsContainer);
  adjustShortcutsAfterToggle(showJumpShortcuts);
}

void CommandPaletteWidget::buildJumpContent(int currentRow, int currentCol,
                                            int maxCol, int lineCount) {
  clearContent();

  prepareJumpState(currentRow, currentCol, maxCol, lineCount);

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

void CommandPaletteWidget::updateJumpPlaceholderHint(
    const QString &placeholder) {
  if (!jumpInput || !jumpPlaceholderHint)
    return;

  jumpPlaceholderHint->setText(placeholder);
  bool shouldShow = jumpInput->text().isEmpty();
  jumpPlaceholderHint->setVisible(shouldShow);
  jumpPlaceholderHint->setGeometry(jumpInput->rect().adjusted(0, 0, 0, 0));
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

  if (obj == jumpInput && event->type() == QEvent::Resize) {
    updateJumpPlaceholderHint(JUMP_PLACEHOLDER_HINT);
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

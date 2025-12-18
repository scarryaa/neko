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
}

CommandPaletteWidget::~CommandPaletteWidget() {}

void CommandPaletteWidget::showEvent(QShowEvent *event) {
  if (!parentWidget())
    return;

  int x = (parentWidget()->width() - this->width()) / 2;
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

  if (text == "le") {
    // Jump to end of current line
    jumpToLineEnd();
    return;
  } else if (text == "lb") {
    // Jump to beginning of current line
    jumpToLineStart();
    return;
  } else if (text == "de") {
    // Jump to document end
    jumpToDocumentEnd();
  } else if (text == "db") {
    // Jump to document beginning
    jumpToDocumentStart();
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
  currentMode = Mode::None;
}

void CommandPaletteWidget::buildJumpContent(int currentRow, int currentCol,
                                            int maxCol, int lineCount) {
  clearContent();
  currentMode = Mode::GoToPosition;
  maxLineCount = std::max(1, lineCount);
  this->maxColumn = std::max(1, maxCol);
  this->maxRow = std::max(1, lineCount);
  int clampedRow = std::clamp(currentRow, 0, maxLineCount - 1);
  int clampedCol = std::clamp(currentCol, 0, maxColumn - 1);
  this->currentRow = clampedRow;

  auto foregroundColor = UiUtils::getThemeColor(themeManager, "ui.foreground");
  auto foregroundVeryMutedColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground.very_muted");
  auto font = UiUtils::loadFont(configManager, neko::FontType::Interface);
  font.setPointSizeF(20.0);

  QString jumpStyleSheet("color: %1; border: 0px; background: transparent;");
  jumpInput = new QLineEdit(mainFrame);
  jumpInput->setFont(font);
  QString displayPos = QString("%1:%2").arg(clampedRow + 1).arg(clampedCol + 1);
  jumpInput->setPlaceholderText(displayPos);
  jumpInput->setStyleSheet(jumpStyleSheet.arg(foregroundColor) +
                           QString("padding-left: 12px; padding-right: 12px;"));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  auto *topSpacer =
      new QSpacerItem(0, 8, QSizePolicy::Minimum, QSizePolicy::Fixed);
  frameLayout->addItem(topSpacer);
  frameLayout->addWidget(jumpInput);
  auto *bottomSpacer =
      new QSpacerItem(0, 12, QSizePolicy::Minimum, QSizePolicy::Fixed);

  auto borderColor =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  auto *divider = new PaletteDivider(borderColor, mainFrame);

  divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  divider->setFixedWidth(WIDTH / 1.5);
  divider->setFixedHeight(1);
  divider->setStyleSheet(QString("background-color: %1;").arg(borderColor));
  frameLayout->addWidget(divider);

  QString styleSheet("color: %1; border: 0px; padding-left: 12px; "
                     "padding-right: 12px;");
  font.setPointSizeF(18.0);
  auto *label =
      UiUtils::createLabel(QString("Current line: %1 of %2 (column %3)")
                               .arg(clampedRow + 1)
                               .arg(maxLineCount)
                               .arg(clampedCol + 1),
                           styleSheet.arg(foregroundColor), font, mainFrame,
                           false, QSizePolicy::Fixed, QSizePolicy::Fixed);

  QFont shortcutFont = font;
  QWidget *shortcutsRow = new QWidget(mainFrame);
  auto *shortcutsRowLayout = new QHBoxLayout(shortcutsRow);
  shortcutsRowLayout->setContentsMargins(0, 0, 0, 0);
  shortcutsRowLayout->setSpacing(6);

  shortcutsToggle = new QToolButton(mainFrame);
  shortcutsToggle->setText("  Shortcuts");
  shortcutsToggle->setCheckable(true);
  shortcutsToggle->setChecked(showJumpShortcuts);
  shortcutsToggle->setArrowType(Qt::DownArrow);
  shortcutsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  shortcutsToggle->setFont(shortcutFont);
  shortcutsToggle->setStyleSheet(
      QString("QToolButton { color: %1; border: none; background: transparent; "
              "padding-left: 16px; padding-right: 16px; }"
              "QToolButton:hover { color: %2; }")
          .arg(foregroundColor, foregroundVeryMutedColor));
  shortcutsToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  shortcutsRowLayout->addWidget(shortcutsToggle);

  shortcutsContainer = new QWidget(mainFrame);
  auto *shortcutsLayout = new QVBoxLayout(shortcutsContainer);
  shortcutsLayout->setContentsMargins(4, 0, 4, 0);
  shortcutsLayout->setSpacing(2);
  adjustShortcutsAfterToggle(showJumpShortcuts);

  // TODO: Allow command aliases?
  auto *leHintLabel = UiUtils::createLabel(
      QString("le - current line end"),
      styleSheet.arg(foregroundVeryMutedColor), shortcutFont, mainFrame, false,
      QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *lbHintLabel = UiUtils::createLabel(
      QString("lb - current line beginning"),
      styleSheet.arg(foregroundVeryMutedColor), shortcutFont, mainFrame, false,
      QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *deHintLabel = UiUtils::createLabel(
      QString("de - document end"), styleSheet.arg(foregroundVeryMutedColor),
      shortcutFont, mainFrame, false, QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *dbHintLabel = UiUtils::createLabel(
      QString("db - document beginning"),
      styleSheet.arg(foregroundVeryMutedColor), shortcutFont, mainFrame, false,
      QSizePolicy::Fixed, QSizePolicy::Fixed);

  shortcutsLayout->addWidget(leHintLabel);
  shortcutsLayout->addWidget(lbHintLabel);
  shortcutsLayout->addWidget(deHintLabel);
  shortcutsLayout->addWidget(dbHintLabel);

  auto *topLabelSpacer =
      new QSpacerItem(0, 4, QSizePolicy::Minimum, QSizePolicy::Fixed);
  frameLayout->addItem(topLabelSpacer);
  frameLayout->addWidget(label);
  frameLayout->addWidget(shortcutsRow);
  frameLayout->addWidget(shortcutsContainer);
  auto *bottomLabelSpacer =
      new QSpacerItem(0, 12, QSizePolicy::Minimum, QSizePolicy::Fixed);
  frameLayout->addItem(bottomLabelSpacer);

  connect(shortcutsToggle, &QToolButton::toggled, this,
          [this](bool checked) { adjustShortcutsAfterToggle(checked); });

  connect(jumpInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitJumpRequestFromInput);
  adjustSize();
}

void CommandPaletteWidget::adjustShortcutsAfterToggle(bool checked) {
  shortcutsContainer->setVisible(checked);
  shortcutsToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
  shortcutsContainer->adjustSize();
  shortcutsToggle->updateGeometry();
  shortcutsContainer->updateGeometry();
  showJumpShortcuts = checked;
  this->adjustSize();
}

bool CommandPaletteWidget::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    if (!this->rect().contains(
            this->mapFromGlobal(mouseEvent->globalPosition().toPoint()))) {
      this->close();
      return true;
    }
  }

  return false;
}

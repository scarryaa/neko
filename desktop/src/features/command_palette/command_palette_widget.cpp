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
      "} QFrame{ border-radius: 12px; background: %1; border: 1.5px "
      "solid %2; }";
  setStyleSheet(stylesheet.arg(backgroundColor, borderColor));
  setFixedSize(WIDTH, HEIGHT);

  mainFrame = new QFrame(this);

  QVBoxLayout *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN,
                                 CONTENT_MARGIN);
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
                                           int lineCount) {
  buildJumpContent(currentRow, currentCol, lineCount);
  show();
  if (jumpInput) {
    jumpInput->setFocus();
    jumpInput->selectAll();
  }
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

  int row = parts.value(0).toInt(&okRow);
  int col = parts.size() > 1 ? parts.value(1).toInt(&okCol) : 1;

  if (!okRow || !okCol || row <= 0 || col <= 0) {
    jumpInput->selectAll();
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
                                            int lineCount) {
  clearContent();
  currentMode = Mode::GoToPosition;
  maxLineCount = std::max(1, lineCount);
  int clampedRow = std::clamp(currentRow, 0, maxLineCount - 1);
  int clampedCol = std::max(0, currentCol);

  auto foregroundColor = UiUtils::getThemeColor(themeManager, "ui.foreground");
  auto font = UiUtils::loadFont(configManager, neko::FontType::Interface);

  QString jumpStyleSheet("color: %1; border: 0px; background: transparent;");
  jumpInput = new QLineEdit(mainFrame);
  jumpInput->setFont(font);
  jumpInput->setPlaceholderText(
      QString("%1:%2").arg(clampedRow + 1).arg(clampedCol + 1));
  jumpInput->setStyleSheet(jumpStyleSheet.arg(foregroundColor));
  jumpInput->setClearButtonEnabled(false);
  jumpInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  jumpInput->setMinimumHeight(30.0);
  auto *topSpacer =
      new QSpacerItem(0, 8, QSizePolicy::Minimum, QSizePolicy::Fixed);
  frameLayout->addItem(topSpacer);
  frameLayout->addWidget(jumpInput);
  auto *bottomSpacer =
      new QSpacerItem(0, 4, QSizePolicy::Minimum, QSizePolicy::Fixed);
  frameLayout->addItem(bottomSpacer);

  auto borderColor =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  auto *divider = new QFrame(mainFrame);
  divider->setFrameShape(QFrame::NoFrame);
  divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  divider->setFixedWidth(WIDTH);
  divider->setFixedHeight(1);
  divider->setStyleSheet(QString("background-color: %1;").arg(borderColor));
  frameLayout->addWidget(divider);

  auto *label = new QLabel(QString("Current line: %1 of %2 (column %3)")
                               .arg(clampedRow + 1)
                               .arg(maxLineCount)
                               .arg(clampedCol + 1),
                           mainFrame);
  QString styleSheet("color: %1; border: 0px;");
  label->setStyleSheet(styleSheet.arg(foregroundColor));
  label->setFont(font);
  label->setWordWrap(false);
  label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  frameLayout->addWidget(label);

  connect(jumpInput, &QLineEdit::returnPressed, this,
          &CommandPaletteWidget::emitJumpRequestFromInput);
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

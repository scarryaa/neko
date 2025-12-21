#include "status_bar_widget.h"

StatusBarWidget::StatusBarWidget(EditorController *editorController,
                                 neko::ConfigManager &configManager,
                                 neko::ThemeManager &themeManager,
                                 QWidget *parent)
    : QWidget(parent), configManager(configManager),
      editorController(editorController), themeManager(themeManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;
  m_height = dynamicHeight;
  setFixedHeight(m_height);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  auto snapshot = configManager.get_config_snapshot();
  bool fileExplorerShown = snapshot.file_explorer_shown;

  fileExplorerToggleButton = new QPushButton();
  fileExplorerToggleButton->setCheckable(true);
  fileExplorerToggleButton->setChecked(fileExplorerShown);

  connect(fileExplorerToggleButton, &QPushButton::clicked, this,
          &StatusBarWidget::onFileExplorerToggled);

  cursorPosition = new QPushButton(this);

  connect(cursorPosition, &QPushButton::clicked, this,
          &StatusBarWidget::onCursorPositionClicked);

  QHBoxLayout *layout = new QHBoxLayout(this);

  layout->setContentsMargins(10, 5, 10, 5);
  layout->addWidget(fileExplorerToggleButton);
  layout->addWidget(cursorPosition);
  layout->addStretch();

  applyTheme();
}

StatusBarWidget::~StatusBarWidget() {}

void StatusBarWidget::applyTheme() {
  QString btnText = UiUtils::getThemeColor(
      themeManager, "titlebar.button.foreground", "#a0a0a0");
  QString btnHover =
      UiUtils::getThemeColor(themeManager, "titlebar.button.hover", "#131313");
  QString btnPress = UiUtils::getThemeColor(
      themeManager, "titlebar.button.pressed", "#222222");

  QColor greyColor =
      UiUtils::getThemeColor(themeManager, "ui.foreground.muted");
  QColor accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  QSize iconSize(18, 18);

  QIcon baseIcon =
      QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
  QIcon toggleIcon;

  QIcon greyIcon = UiUtils::createColorizedIcon(baseIcon, greyColor, iconSize);
  toggleIcon.addPixmap(greyIcon.pixmap(iconSize), QIcon::Normal, QIcon::Off);

  QIcon accentIcon =
      UiUtils::createColorizedIcon(baseIcon, accentColor, iconSize);
  toggleIcon.addPixmap(accentIcon.pixmap(iconSize), QIcon::Normal, QIcon::On);

  fileExplorerToggleButton->setIcon(toggleIcon);
  fileExplorerToggleButton->setIconSize(iconSize);
  fileExplorerToggleButton->setStyleSheet(
      QString("QPushButton {"
              "  background-color: transparent;"
              "  border-radius: 4px;"
              "  padding: 2px 2px;"
              "}"
              "QPushButton:hover { background-color: %1; }"
              "QPushButton:pressed { background-color: %2; }")
          .arg(btnHover, btnPress));

  cursorPosition->setStyleSheet(
      QString("QPushButton {"
              "  background-color: transparent;"
              "  color: %1;"
              "  border-radius: 4px;"
              "  padding: 2px 2px;"
              "}"
              "QPushButton:hover { background-color: %2; }"
              "QPushButton:pressed { background-color: %3; }")
          .arg(btnText, btnHover, btnPress));

  update();
}

void StatusBarWidget::updateCursorPosition(int row, int col,
                                           int numberOfCursors) {
  if (!editorController) {
    return;
  }

  QString newPosition = QString("%1:%2").arg(row + 1).arg(col + 1);

  // TODO: Show selection lines, chars
  int numberOfSelections = editorController->getNumberOfSelections();
  if (numberOfCursors > 1 || numberOfSelections > 1) {
    newPosition = newPosition.append(" (%1 selections)")
                      .arg(std::max(numberOfCursors, numberOfSelections));
  }
  cursorPosition->setText(newPosition);
}

void StatusBarWidget::showCursorPositionInfo() { cursorPosition->show(); }

void StatusBarWidget::onCursorPositionChanged(int row, int col,
                                              int numberOfCursors) {
  if (!editorController) {
    return;
  }

  cursorPosition->show();

  QString newPosition = QString("%1:%2").arg(row + 1).arg(col + 1);

  // TODO: Show selection lines, chars
  int numberOfSelections = editorController->getNumberOfSelections();
  if (numberOfCursors > 1 || numberOfSelections > 1) {
    newPosition = newPosition.append(" (%1 selections)")
                      .arg(std::max(numberOfCursors, numberOfSelections));
  }
  cursorPosition->setText(newPosition);
}

void StatusBarWidget::onTabClosed(int numberOfTabs) {
  if (numberOfTabs == 0) {
    cursorPosition->hide();
  }
}

void StatusBarWidget::onFileExplorerToggledExternally(bool isOpen) {
  fileExplorerToggleButton->setChecked(isOpen);
}

void StatusBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  painter.setBrush(UiUtils::getThemeColor(themeManager, "ui.background"));
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), height())));

  painter.setPen(UiUtils::getThemeColor(themeManager, "ui.border"));
  painter.drawLine(QPointF(0, 0), QPointF(width(), 0));
}

void StatusBarWidget::onFileExplorerToggled() { emit fileExplorerToggled(); }

void StatusBarWidget::onCursorPositionClicked() {
  // Toggle disabled to force clear hover effect
  cursorPosition->setDisabled(true);
  cursorPosition->setDisabled(false);
  emit cursorPositionClicked();
}

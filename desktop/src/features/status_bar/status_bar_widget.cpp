#include "status_bar_widget.h"
#include "features/editor/controllers/editor_controller.h"
#include "utils/ui_utils.h"
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QPushButton>
#include <QRectF>
#include <QString>
#include <QStyle>
#include <QStyleOption>

StatusBarWidget::StatusBarWidget(const StatusBarProps &props, QWidget *parent)
    : QWidget(parent), configManager(*props.configManager),
      editorController(props.editorController), theme(props.theme) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);
  m_height = dynamicHeight;
  setFixedHeight(static_cast<int>(m_height));

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

  auto *layout = new QHBoxLayout(this);

  layout->setContentsMargins(HORIZONTAL_CONTENT_MARGIN, VERTICAL_CONTENT_MARGIN,
                             HORIZONTAL_CONTENT_MARGIN,
                             VERTICAL_CONTENT_MARGIN);
  layout->addWidget(fileExplorerToggleButton);
  layout->addWidget(cursorPosition);
  layout->addStretch();

  setAndApplyTheme(theme);
}

void StatusBarWidget::setAndApplyTheme(const StatusBarTheme &newTheme) {
  theme = newTheme;

  QString btnText = theme.buttonForegroundColor;
  QString btnHover = theme.buttonHoverColor;
  QString btnPress = theme.buttonPressColor;

  QColor greyColor = theme.foregroundMutedColor;
  QColor accentColor = theme.accentColor;
  QSize iconSize(ICON_SIZE, ICON_SIZE);

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

// NOLINTNEXTLINE
void StatusBarWidget::updateCursorPosition(int row, int col,
                                           int numberOfCursors) {
  if (editorController == nullptr) {
    return;
  }

  QString newPosition = QString("%1:%2").arg(row + 1).arg(col + 1);

  // TODO(scarlet): Show selection lines, chars
  int numberOfSelections = editorController->getNumberOfSelections();
  if (numberOfCursors > 1 || numberOfSelections > 1) {
    newPosition = newPosition.append(" (%1 selections)")
                      .arg(std::max(numberOfCursors, numberOfSelections));
  }
  cursorPosition->setText(newPosition);
}

void StatusBarWidget::showCursorPositionInfo() { cursorPosition->show(); }

// NOLINTNEXTLINE
void StatusBarWidget::onCursorPositionChanged(int row, int col,
                                              int numberOfCursors) {
  if (editorController == nullptr) {
    return;
  }

  cursorPosition->show();

  QString newPosition = QString("%1:%2").arg(row + 1).arg(col + 1);

  // TODO(scarlet): Show selection lines, chars
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

  painter.setBrush(theme.backgroundColor);
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), height())));

  painter.setPen(theme.borderColor);
  painter.drawLine(QPointF(0, 0), QPointF(width(), 0));
}

void StatusBarWidget::onFileExplorerToggled() { emit fileExplorerToggled(); }

void StatusBarWidget::onCursorPositionClicked() {
  // Toggle disabled to force clear hover effect
  cursorPosition->setDisabled(true);
  cursorPosition->setDisabled(false);
  emit cursorPositionClicked();
}

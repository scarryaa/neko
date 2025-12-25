#include "theme_connections.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include "theme/theme_provider.h"
#include <QPushButton>
#include <QSplitter>

ThemeConnections::ThemeConnections(const ThemeConnectionsProps &props,
                                   QObject *parent)
    : QObject(parent), uiHandles(props.uiHandles) {
  auto *themeProvider = props.themeProvider;

  // ThemeProvider -> TitleBarWidget
  connect(themeProvider, &ThemeProvider::titleBarThemeChanged,
          uiHandles.titleBarWidget, &TitleBarWidget::setAndApplyTheme);

  // ThemeProvider -> FileExplorerWidget
  connect(themeProvider, &ThemeProvider::fileExplorerThemeChanged,
          uiHandles.fileExplorerWidget, &FileExplorerWidget::setAndApplyTheme);

  // ThemeProvider -> EditorWidget
  connect(themeProvider, &ThemeProvider::editorThemeChanged,
          uiHandles.editorWidget, &EditorWidget::setAndApplyTheme);

  // ThemeProvider -> GutterWidget
  connect(themeProvider, &ThemeProvider::gutterThemeChanged,
          uiHandles.gutterWidget, &GutterWidget::setAndApplyTheme);

  // ThemeProvider -> StatusBarWidget
  connect(themeProvider, &ThemeProvider::statusBarThemeChanged,
          uiHandles.statusBarWidget, &StatusBarWidget::setAndApplyTheme);

  // ThemeProvider -> TabBarWidget
  connect(themeProvider, &ThemeProvider::tabBarThemeChanged,
          uiHandles.tabBarWidget, &TabBarWidget::setAndApplyTheme);
  connect(themeProvider, &ThemeProvider::tabThemeChanged,
          uiHandles.tabBarWidget, &TabBarWidget::setAndApplyTabTheme);

  // ThemeProvider -> CommandPaletteWidget
  connect(themeProvider, &ThemeProvider::commandPaletteThemeChanged,
          uiHandles.commandPaletteWidget,
          &CommandPaletteWidget::setAndApplyTheme);

  // ThemeProvider -> NewTabButton
  connect(themeProvider, &ThemeProvider::newTabButtonThemeChanged, this,
          &ThemeConnections::applyNewTabButtonTheme);

  // ThemeProvider -> Splitter
  connect(themeProvider, &ThemeProvider::splitterThemeChanged, this,
          &ThemeConnections::applySplitterTheme);

  // ThemeProvider -> EmptyStateWidget
  connect(themeProvider, &ThemeProvider::emptyStateThemeChanged, this,
          &ThemeConnections::applyEmptyStateTheme);
}

void ThemeConnections::applyNewTabButtonTheme(
    const NewTabButtonTheme &theme) const {
  if (uiHandles.newTabButton == nullptr) {
    return;
  }

  const QString styleSheet =
      QString("QPushButton {"
              "  background: %1;"
              "  color: %2;"
              "  border: none;"
              "  border-left: 1px solid %4;"
              "  border-bottom: 1px solid %4;"
              "  font-size: 20px;"
              "}"
              "QPushButton:hover {"
              "  background: %3;"
              "}")
          .arg(theme.backgroundColor, theme.foregroundColor, theme.hoverColor,
               theme.borderColor);

  uiHandles.newTabButton->setStyleSheet(styleSheet);
}

void ThemeConnections::applySplitterTheme(const SplitterTheme &theme) const {
  if (uiHandles.mainSplitter == nullptr) {
    return;
  }

  const QString styleSheet = QString("QSplitter::handle {"
                                     "  background-color: %1;"
                                     "  margin: 0px;"
                                     "}")
                                 .arg(theme.handleColor);

  uiHandles.mainSplitter->setHandleWidth(theme.handleWidth);
  uiHandles.mainSplitter->setStyleSheet(styleSheet);
}

// TODO(scarlet): Disable fade in on new theme
void ThemeConnections::applyEmptyStateTheme(
    const EmptyStateTheme &theme) const {
  if (uiHandles.emptyStateWidget == nullptr) {
    return;
  }

  // Container style
  const QString styleSheet =
      QString(
          "QWidget { background-color: %1; }"
          "QPushButton { background-color: %2; border-radius: 4px; color: %3; }"
          "QPushButton:hover { background-color: %4; }")
          .arg(theme.backgroundColor, theme.buttonBackgroundColor,
               theme.buttonForegroundColor, theme.buttonHoverColor);

  uiHandles.emptyStateWidget->setStyleSheet(styleSheet);
}

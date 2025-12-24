#include "theme_connections.h"
#include "theme/theme_provider.h"

ThemeConnections::ThemeConnections(const WorkspaceUiHandles &uiHandles,
                                   ThemeProvider *themeProvider,
                                   QObject *parent)
    : QObject(parent), uiHandles(uiHandles) {
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

  const QString qss = QString("QSplitter::handle {"
                              "  background-color: %1;"
                              "  margin: 0px;"
                              "}")
                          .arg(theme.handleColor);

  uiHandles.mainSplitter->setHandleWidth(theme.handleWidth);
  uiHandles.mainSplitter->setStyleSheet(qss);
}

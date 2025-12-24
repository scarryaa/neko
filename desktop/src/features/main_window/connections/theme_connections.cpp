#include "theme_connections.h"

ThemeConnections::ThemeConnections(const WorkspaceUiHandles &uiHandles,
                                   ThemeManager *themeManager,
                                   ThemeProvider *themeProvider,
                                   QObject *parent)
    : QObject(parent) {
  // ThemeManager -> TitleBarWidget
  connect(themeProvider, &ThemeProvider::titleBarThemeChanged,
          uiHandles.titleBarWidget, &TitleBarWidget::setAndApplyTheme);

  // ThemeManager -> FileExplorerWidget
  connect(themeManager, &ThemeManager::themeChanged,
          uiHandles.fileExplorerWidget, &FileExplorerWidget::applyTheme);

  // ThemeManager -> EditorWidget
  connect(themeManager, &ThemeManager::themeChanged, uiHandles.editorWidget,
          &EditorWidget::applyTheme);

  // ThemeManager -> GutterWidget
  connect(themeManager, &ThemeManager::themeChanged, uiHandles.gutterWidget,
          &GutterWidget::applyTheme);

  // ThemeManager -> StatusBarWidget
  connect(themeManager, &ThemeManager::themeChanged, uiHandles.statusBarWidget,
          &StatusBarWidget::applyTheme);

  // ThemeManager -> TabBarWidget
  connect(themeManager, &ThemeManager::themeChanged, uiHandles.tabBarWidget,
          &TabBarWidget::applyTheme);

  // ThemeManager -> CommandPaletteWidget
  connect(themeManager, &ThemeManager::themeChanged,
          uiHandles.commandPaletteWidget, &CommandPaletteWidget::applyTheme);
}

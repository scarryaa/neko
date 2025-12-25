#include "theme/theme_provider.h"
#include "theme/theme_types.h"
#include "utils/gui_utils.h"

ThemeProvider::ThemeProvider(const ThemeProviderProps &props, QObject *parent)
    : QObject(parent), themeManager(props.themeManager) {}

const TitleBarTheme &ThemeProvider::getTitleBarTheme() const {
  return titleBarTheme;
}

const StatusBarTheme &ThemeProvider::getStatusBarTheme() const {
  return statusBarTheme;
}

const FileExplorerTheme &ThemeProvider::getFileExplorerTheme() const {
  return fileExplorerTheme;
}

const CommandPaletteTheme &ThemeProvider::getCommandPaletteTheme() const {
  return commandPaletteTheme;
}

const TabBarTheme &ThemeProvider::getTabBarTheme() const { return tabBarTheme; }

const TabTheme &ThemeProvider::getTabTheme() const { return tabTheme; }

const EditorTheme &ThemeProvider::getEditorTheme() const { return editorTheme; }

const GutterTheme &ThemeProvider::getGutterTheme() const { return gutterTheme; }

const ScrollBarTheme &ThemeProvider::getScrollBarTheme() const {
  return scrollBarTheme;
}

const NewTabButtonTheme &ThemeProvider::getNewTabButtonTheme() const {
  return newTabButtonTheme;
}

const SplitterTheme &ThemeProvider::getSplitterTheme() const {
  return splitterTheme;
}

const EmptyStateTheme &ThemeProvider::getEmptyStateTheme() const {
  return emptyStateTheme;
}

const ContextMenuTheme &ThemeProvider::getContextMenuTheme() const {
  return contextMenuTheme;
}

void ThemeProvider::reload() {
  if (themeManager == nullptr) {
    return;
  }

  refreshScrollBarTheme();
  refreshTitleBarTheme();
  refreshFileExplorerTheme();
  refreshCommandPaletteTheme();
  refreshTabTheme();
  refreshTabBarTheme();
  refreshEditorTheme();
  refreshGutterTheme();
  refreshStatusBarTheme();
  refreshNewTabButtonTheme();
  refreshSplitterTheme();
  refreshEmptyStateTheme();
  refreshContextMenuTheme();
}

void ThemeProvider::refreshTitleBarTheme() {
  auto [titleBarButtonForeground, titleBarButtonHover, titleBarButtonPressed,
        titleBarBackground, titleBarBorder] =
      UiUtils::getThemeColors(
          *themeManager, "titlebar.button.foreground", "titlebar.button.hover",
          "titlebar.button.pressed", "ui.background", "ui.border");

  TitleBarTheme newTheme{titleBarButtonForeground, titleBarButtonHover,
                         titleBarButtonPressed, titleBarBackground,
                         titleBarBorder};

  titleBarTheme = newTheme;
  emit titleBarThemeChanged(titleBarTheme);
}

void ThemeProvider::refreshFileExplorerTheme() {
  auto [backgroundColor, buttonBackgroundColor, buttonForegroundColor,
        buttonHoverColor, buttonPressColor, fileForegroundColor,
        fileHiddenColor, selectionColor] =
      UiUtils::getThemeColors(
          *themeManager, "file_explorer.background", "ui.accent",
          "ui.accent.foreground", "ui.accent.hover", "ui.accent.pressed",
          "ui.foreground", "ui.foreground.very_muted", "ui.accent");

  FileExplorerTheme newTheme{
      backgroundColor,  buttonBackgroundColor, buttonForegroundColor,
      buttonHoverColor, buttonPressColor,      fileForegroundColor,
      fileHiddenColor,  selectionColor,        scrollBarTheme};

  fileExplorerTheme = newTheme;
  emit fileExplorerThemeChanged(fileExplorerTheme);
}

void ThemeProvider::refreshCommandPaletteTheme() {
  auto [backgroundColor, borderColor, foregroundColor, foregroundVeryMutedColor,
        accentMutedColor, accentForegroundColor, shadowColor] =
      UiUtils::getThemeColors(*themeManager, "command_palette.background",
                              "command_palette.border", "ui.foreground",
                              "ui.foreground.very_muted", "ui.accent.muted",
                              "ui.accent.foreground", "command_palette.shadow");

  CommandPaletteTheme newTheme{
      backgroundColor,  borderColor,
      foregroundColor,  foregroundVeryMutedColor,
      accentMutedColor, accentForegroundColor,
      shadowColor,
  };

  commandPaletteTheme = newTheme;
  emit commandPaletteThemeChanged(commandPaletteTheme);
}

void ThemeProvider::refreshTabBarTheme() {
  auto [backgroundColor, indicatorColor, borderColor] = UiUtils::getThemeColors(
      *themeManager, "tab_bar.background", "ui.accent", "ui.border");

  TabBarTheme newTheme{backgroundColor, indicatorColor, borderColor,
                       scrollBarTheme};

  tabBarTheme = newTheme;
  emit tabBarThemeChanged(tabBarTheme);
}

void ThemeProvider::refreshTabTheme() {
  auto [tabForegroundColor, tabForegroundInactiveColor, tabActiveColor,
        tabInactiveColor, tabHoverColor, tabModifiedIndicatorColor,
        tabCloseButtonHoverColor, borderColor] =
      UiUtils::getThemeColors(*themeManager, "ui.foreground",
                              "ui.foreground.muted", "tab.active",
                              "tab.inactive", "tab.hover", "ui.accent",
                              "ui.background.hover", "ui.border");

  TabTheme newTheme{tabForegroundColor,
                    tabForegroundInactiveColor,
                    tabActiveColor,
                    tabInactiveColor,
                    tabHoverColor,
                    tabModifiedIndicatorColor,
                    tabCloseButtonHoverColor,
                    borderColor};

  tabTheme = newTheme;
  emit tabThemeChanged(tabTheme);
}

void ThemeProvider::refreshEditorTheme() {
  auto [backgroundColor, foregroundColor, highlightColor, accentColor] =
      UiUtils::getThemeColors(*themeManager, "editor.background",
                              "editor.foreground", "editor.highlight",
                              "ui.accent");

  EditorTheme newTheme{backgroundColor, foregroundColor, highlightColor,
                       accentColor, scrollBarTheme};

  editorTheme = newTheme;
  emit editorThemeChanged(editorTheme);
}

void ThemeProvider::refreshGutterTheme() {
  auto [backgroundColor, foregroundColor, foregroundActiveColor, accentColor,
        highlightColor] =
      UiUtils::getThemeColors(
          *themeManager, "editor.gutter.background", "editor.gutter.foreground",
          "editor.gutter.foreground.active", "ui.accent", "editor.highlight");

  GutterTheme newTheme{backgroundColor, foregroundColor, foregroundActiveColor,
                       accentColor, highlightColor};

  gutterTheme = newTheme;
  emit gutterThemeChanged(gutterTheme);
}

void ThemeProvider::refreshStatusBarTheme() {
  auto [backgroundColor, borderColor, buttonForegroundColor, buttonHoverColor,
        buttonPressColor, foregroundMutedColor, accentColor] =
      UiUtils::getThemeColors(
          *themeManager, "ui.background", "ui.border",
          "titlebar.button.foreground", "titlebar.button.hover",
          "titlebar.button.pressed", "ui.foreground.muted", "ui.accent");

  StatusBarTheme newTheme{
      backgroundColor,  borderColor,      buttonForegroundColor,
      buttonHoverColor, buttonPressColor, foregroundMutedColor,
      accentColor};

  statusBarTheme = newTheme;
  emit statusBarThemeChanged(statusBarTheme);
}

void ThemeProvider::refreshScrollBarTheme() {
  auto [scrollBarThumbColor, scrollBarThumbHoverColor] =
      UiUtils::getThemeColors(*themeManager, "ui.scrollbar.thumb",
                              "ui.scrollbar.thumb.hover");

  ScrollBarTheme newTheme{scrollBarThumbColor, scrollBarThumbHoverColor};

  scrollBarTheme = newTheme;
  emit scrollBarThemeChanged(scrollBarTheme);
}

void ThemeProvider::refreshContextMenuTheme() {
  auto [backgroundColor, borderColor, labelColor, labelDisabledColor,
        shortcutColor, shortcutDisabledColor, hoverColor, accentMutedColor,
        accentForegroundColor, shadowColor] =

      UiUtils::getThemeColors(
          *themeManager, "context_menu.background", "context_menu.border",
          "context_menu.label", "context_menu.label.disabled",
          "context_menu.shortcut", "context_menu.shortcut.disabled",
          "context_menu.button.hover", "ui.accent.muted",
          "ui.accent.foreground", "context_menu.shadow");

  ContextMenuTheme newTheme{
      backgroundColor,    borderColor,      labelColor,
      labelDisabledColor, shortcutColor,    shortcutDisabledColor,
      hoverColor,         accentMutedColor, accentForegroundColor,
      shadowColor,
  };

  contextMenuTheme = newTheme;
  emit contextMenuThemeChanged(contextMenuTheme);
}

void ThemeProvider::refreshNewTabButtonTheme() {
  auto [backgroundColor, foregroundColor, hoverColor, borderColor] =
      UiUtils::getThemeColors(*themeManager, "ui.background", "ui.foreground",
                              "ui.background.hover", "ui.border");

  NewTabButtonTheme newTheme{backgroundColor, foregroundColor, hoverColor,
                             borderColor};

  newTabButtonTheme = newTheme;
  emit newTabButtonThemeChanged(newTabButtonTheme);
}

void ThemeProvider::refreshSplitterTheme() {
  auto [handleColor] = UiUtils::getThemeColors(*themeManager, "ui.border");

  SplitterTheme newTheme{handleColor, 1};

  splitterTheme = newTheme;
  emit splitterThemeChanged(splitterTheme);
}

void ThemeProvider::refreshEmptyStateTheme() {
  auto [backgroundColor, buttonBackgroundColor, foregroundColor, hoverColor,
        borderColor] =
      UiUtils::getThemeColors(*themeManager, "ui.background", "ui.accent.muted",
                              "ui.foreground", "ui.background.hover",
                              "ui.border");

  EmptyStateTheme newTheme{backgroundColor, buttonBackgroundColor,
                           foregroundColor, hoverColor, borderColor};

  emptyStateTheme = newTheme;
  emit emptyStateThemeChanged(emptyStateTheme);
}

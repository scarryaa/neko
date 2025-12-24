#include "theme/theme_provider.h"
#include "theme/theme_types.h"
#include "utils/gui_utils.h"

ThemeProvider::ThemeProvider(neko::ThemeManager *themeManager, QObject *parent)
    : QObject(parent), themeManager(themeManager) {}

const TitleBarTheme &ThemeProvider::getTitleBarTheme() const {
  return titleBarTheme;
}

void ThemeProvider::reload() {
  if (themeManager == nullptr) {
    return;
  }

  refreshTitleBarTheme();
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

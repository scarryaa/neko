#include "theme_manager.h"
#include "utils/gui_utils.h"

ThemeManager::ThemeManager(neko::ThemeManager *nekoThemeManager,
                           const WorkspaceUiHandles *uiHandles, QObject *parent)
    : nekoThemeManager(nekoThemeManager), uiHandles(uiHandles),
      QObject(parent) {}

void ThemeManager::applyTheme(const std::string &themeName) {
  if (themeName.empty()) {
    return;
  }

  nekoThemeManager->set_theme(themeName);
  emit themeChanged();

  if (uiHandles->newTabButton != nullptr) {
    QString newTabButtonBackgroundColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.background");
    QString newTabButtonForegroundColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.foreground");
    QString newTabButtonBorderColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.border");
    QString newTabButtonHoverBackgroundColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.background.hover");

    QString newTabButtonStylesheet =
        QString("QPushButton {"
                "  background: %1;"
                "  color: %2;"
                "  border: none;"
                "  border-left: 1px solid %3;"
                "  border-bottom: 1px solid %3;"
                "  font-size: 20px;"
                "}"
                "QPushButton:hover {"
                "  background: %4;"
                "}")
            .arg(newTabButtonBackgroundColor, newTabButtonForegroundColor,
                 newTabButtonBorderColor, newTabButtonHoverBackgroundColor);
    uiHandles->newTabButton->setStyleSheet(newTabButtonStylesheet);
  }

  if (uiHandles->emptyStateWidget != nullptr) {
    QString accentMutedColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.accent.muted");
    QString foregroundColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.foreground");
    QString emptyStateBackgroundColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.background");

    QString emptyStateStylesheet =
        QString("QWidget { background-color: %1; }"
                "QPushButton { background-color: %2; border-radius: 4px; "
                "color: %3; }")
            .arg(emptyStateBackgroundColor, accentMutedColor, foregroundColor);
    uiHandles->emptyStateWidget->setStyleSheet(emptyStateStylesheet);
  }

  if (uiHandles->mainSplitter != nullptr) {
    QString borderColor =
        UiUtils::getThemeColor(*nekoThemeManager, "ui.border");
    QString splitterStylesheet = QString("QSplitter::handle {"
                                         "  background-color: %1;"
                                         "  margin: 0px;"
                                         "}")
                                     .arg(borderColor);
    uiHandles->mainSplitter->setStyleSheet(splitterStylesheet);
  }
}

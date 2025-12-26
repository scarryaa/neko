#include "ui_style_connections.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/editor_widget.h"
#include "features/main_window/controllers/app_config_service.h"
#include "features/main_window/controllers/ui_style_manager.h"

UiStyleConnections::UiStyleConnections(const UiStyleConnectionsProps &props,
                                       QObject *parent)
    : QObject(parent) {
  auto *uiStyleManager = props.uiStyleManager;
  auto uiHandles = props.uiHandles;
  auto *appConfigService = props.appConfigService;

  // TODO(scarlet): Add connections for more widgets
  // UiStyleManager -> CommandPaletteWidget
  connect(uiStyleManager, &UiStyleManager::commandPaletteFontChanged,
          uiHandles.commandPaletteWidget, &CommandPaletteWidget::setFont);

  // AppConfigService -> UiStyleManager
  connect(appConfigService, &AppConfigService::configChanged, uiStyleManager,
          &UiStyleManager::handleConfigChanged);

  // UiStyleManager -> EditorWidget
  connect(uiStyleManager, &UiStyleManager::editorFontChanged,
          uiHandles.editorWidget, &EditorWidget::updateFont);

  // EditorWidget -> UiStyleManager
  connect(uiHandles.editorWidget, &EditorWidget::fontSizeChangedByUser,
          uiStyleManager, &UiStyleManager::onEditorFontSizeChangedByUser);
}

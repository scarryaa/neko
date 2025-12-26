#include "ui_style_connections.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/main_window/controllers/ui_style_manager.h"

UiStyleConnections::UiStyleConnections(const UiStyleConnectionsProps &props,
                                       QObject *parent)
    : QObject(parent) {
  auto *uiStyleManager = props.uiStyleManager;
  auto uiHandles = props.uiHandles;

  // TODO(scarlet): Add connections for more widgets
  connect(uiStyleManager, &UiStyleManager::interfaceFontChanged,
          uiHandles.commandPaletteWidget, &CommandPaletteWidget::updateFont);
  connect(uiStyleManager, &UiStyleManager::commandPaletteFontChanged,
          uiHandles.commandPaletteWidget, &CommandPaletteWidget::setFont);
}

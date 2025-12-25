#include "command_manager.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "neko-core/src/ffi/bridge.rs.h"

CommandManager::CommandManager(CommandRegistry *commandRegistry,
                               ContextMenuRegistry *contextMenuRegistry,
                               WorkspaceCoordinator *workspaceCoordinator,
                               AppStateController *appStateController)
    : commandRegistry(commandRegistry),
      contextMenuRegistry(contextMenuRegistry),
      workspaceCoordinator(workspaceCoordinator),
      appStateController(appStateController) {}

void CommandManager::registerCommands() {
  // TODO(scarlet): Clean this up
  commandRegistry->registerCommand(
      "tab.close", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        auto tabId = static_cast<int>(ctx.id);
        workspaceCoordinator->closeTab(tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeOthers", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        auto tabId = static_cast<int>(ctx.id);
        workspaceCoordinator->closeOtherTabs(tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeLeft", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        auto tabId = static_cast<int>(ctx.id);
        workspaceCoordinator->closeLeftTabs(tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeRight", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        auto tabId = static_cast<int>(ctx.id);
        workspaceCoordinator->closeRightTabs(tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.copyPath", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();

        auto tabId = static_cast<int>(ctx.id);
        workspaceCoordinator->tabCopyPath(tabId);
      });
  commandRegistry->registerCommand(
      "tab.reveal", [this](const QVariant &variant) {
        auto ctx = variant.value<neko::TabContextFfi>();

        workspaceCoordinator->tabReveal("tab.reveal", ctx);
      });
}

void CommandManager::registerProviders() {
  // TODO(scarlet): Centralize this
  // TODO(scarlet): Populate by pulling available commands from rust
  contextMenuRegistry->registerProvider("tab", [this](const QVariant &variant) {
    const auto ctx = variant.value<neko::TabContextFfi>();
    auto state = appStateController->getTabCommandState(ctx);

    QVector<ContextMenuItem> items;

    items.push_back({ContextMenuItemKind::Action, "tab.close", "Close",
                     "Ctrl+W", "", state.can_close});
    items.push_back({ContextMenuItemKind::Action, "tab.closeOthers",
                     "Close Others", "", "", state.can_close_others});

    items.push_back({ContextMenuItemKind::Separator});

    items.push_back({ContextMenuItemKind::Action, "tab.closeLeft", "Close Left",
                     "", "", state.can_close_left});
    items.push_back({ContextMenuItemKind::Action, "tab.closeRight",
                     "Close Right", "", "", state.can_close_right});

    items.push_back({ContextMenuItemKind::Separator});

    ContextMenuItem pin;
    pin.kind = ContextMenuItemKind::Action;
    pin.id = "tab.pin";
    pin.label = ctx.is_pinned ? "Unpin" : "Pin";
    pin.enabled = true;
    pin.checked = state.is_pinned;
    items.push_back(pin);

    items.push_back({ContextMenuItemKind::Separator});

    items.push_back({ContextMenuItemKind::Action, "tab.copyPath", "Copy Path",
                     "", "", state.can_copy_path});
    items.push_back({ContextMenuItemKind::Action, "tab.reveal",
                     "Reveal in Explorer", "", "", state.can_reveal});

    return items;
  });
}

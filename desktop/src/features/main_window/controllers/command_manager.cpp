#include "command_manager.h"
#include "features/context_menu/providers/tab_context.h"

CommandManager::CommandManager(CommandRegistry *commandRegistry,
                               ContextMenuRegistry *contextMenuRegistry,
                               WorkspaceCoordinator *workspaceCoordinator)
    : commandRegistry(commandRegistry),
      contextMenuRegistry(contextMenuRegistry),
      workspaceCoordinator(workspaceCoordinator) {}

void CommandManager::registerCommands() {
  // TODO(scarlet): Clean this up
  commandRegistry->registerCommand(
      "tab.close", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        workspaceCoordinator->closeTab(ctx.tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeOthers", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        workspaceCoordinator->closeOtherTabs(ctx.tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeLeft", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        workspaceCoordinator->closeLeftTabs(ctx.tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.closeRight", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        auto shiftPressed = QApplication::keyboardModifiers().testFlag(
            Qt::KeyboardModifier::ShiftModifier);

        workspaceCoordinator->closeRightTabs(ctx.tabId, shiftPressed);
      });
  commandRegistry->registerCommand(
      "tab.copyPath", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        workspaceCoordinator->tabCopyPath(ctx.tabId);
      });
  commandRegistry->registerCommand("tab.reveal",
                                   [this](const QVariant &variant) {
                                     auto ctx = variant.value<TabContext>();
                                     workspaceCoordinator->tabReveal(ctx.tabId);
                                   });
}

void CommandManager::registerProviders() {
  // TODO(scarlet): Centralize this
  contextMenuRegistry->registerProvider("tab", [](const QVariant &variant) {
    const auto ctx = variant.value<TabContext>();

    QVector<ContextMenuItem> items;

    items.push_back({ContextMenuItemKind::Action, "tab.close", "Close",
                     "Ctrl+W", "", true});
    items.push_back({ContextMenuItemKind::Action, "tab.closeOthers",
                     "Close Others", "", "", ctx.canCloseOthers});
    items.push_back({ContextMenuItemKind::Action, "tab.closeLeft",
                     "Close Tabs to the Left", "", "", true});
    items.push_back({ContextMenuItemKind::Action, "tab.closeRight",
                     "Close Tabs to the Right", "", "", true});

    items.push_back({ContextMenuItemKind::Separator});

    ContextMenuItem pin;
    pin.kind = ContextMenuItemKind::Action;
    pin.id = "tab.pin";
    pin.label = ctx.isPinned ? "Unpin" : "Pin";
    pin.enabled = true;
    pin.checked = ctx.isPinned;
    items.push_back(pin);

    items.push_back({ContextMenuItemKind::Separator});

    items.push_back({ContextMenuItemKind::Action, "tab.copyPath", "Copy Path",
                     "", "", !ctx.filePath.isEmpty()});
    items.push_back({ContextMenuItemKind::Action, "tab.reveal",
                     "Reveal in Explorer", "", "", !ctx.filePath.isEmpty()});

    return items;
  });
}

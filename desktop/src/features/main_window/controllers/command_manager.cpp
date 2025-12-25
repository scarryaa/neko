#include "command_manager.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <cstdint>
#include <map>

namespace {
enum class TabCommandGroup : uint8_t {
  ClosePrimary = 0,
  CloseSides = 1,
  Pin = 2,
  Path = 3,
};

struct TabCommandSpec {
  const char *id;
  const char *label;
  const char *shortcut;
  const char *iconKey;
  TabCommandGroup group;
};

namespace k {
// NOLINTNEXTLINE
const TabCommandSpec tabCommandSpecs[] = {
    {"tab.close", "Close", "Ctrl+W", "", TabCommandGroup::ClosePrimary},
    {"tab.closeOthers", "Close Others", "", "", TabCommandGroup::ClosePrimary},
    {"tab.closeLeft", "Close Left", "", "", TabCommandGroup::CloseSides},
    {"tab.closeRight", "Close Right", "", "", TabCommandGroup::CloseSides},
    {"tab.pin", "Pin", "", "", TabCommandGroup::Pin},
    {"tab.copyPath", "Copy Path", "", "", TabCommandGroup::Path},
    {"tab.reveal", "Reveal in Explorer", "", "", TabCommandGroup::Path},
};
} // namespace k

inline void addItem(QVector<ContextMenuItem> &items, ContextMenuItemKind kind,
                    const QString &commandId, const QString &label,
                    const QString &shortcut, const QString &iconKey,
                    bool enabled = true, bool visible = true,
                    bool checked = false) {
  items.push_back(
      {kind, commandId, label, shortcut, iconKey, enabled, visible, checked});
}

inline void addSeparatorIfNotEmpty(QVector<ContextMenuItem> &items) {
  if (!items.isEmpty()) {
    items.push_back({ContextMenuItemKind::Separator});
  }
}

inline bool isTabCommandEnabled(const std::string &commandId,
                                const neko::TabCommandStateFfi state) {
  std::map<std::string, bool> enabledMap{
      {"tab.close", state.can_close && !state.is_pinned},
      {"tab.closeOthers", state.can_close_others},
      {"tab.closeLeft", state.can_close_left},
      {"tab.closeRight", state.can_close_right},
      {"tab.copyPath", state.can_copy_path},
      {"tab.reveal", state.can_reveal},
      {"tab.pin", true},
  };

  return enabledMap[commandId];
};
} // namespace

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
  contextMenuRegistry->registerProvider("tab", [this](const QVariant &variant) {
    auto availableCommands = AppStateController::getAvailableTabCommands();

    const auto ctx = variant.value<neko::TabContextFfi>();
    const auto state = appStateController->getTabCommandState(ctx);
    QVector<ContextMenuItem> items;

    QSet<QString> availableIds;
    for (auto &command : availableCommands) {
      availableIds.insert(QString::fromStdString(command.id.c_str()));
    }

    bool firstItem = true;
    std::optional<TabCommandGroup> lastGroup;

    for (const auto &commandSpec : k::tabCommandSpecs) {
      const QString commandId = QString::fromUtf8(commandSpec.id);
      if (!availableIds.contains(commandId)) {
        continue;
      }

      // Insert separators when changing groups
      if (!firstItem) {
        if (!lastGroup.has_value() || commandSpec.group != lastGroup.value()) {
          addSeparatorIfNotEmpty(items);
        }
      }

      firstItem = false;
      lastGroup = commandSpec.group;

      bool enabled = isTabCommandEnabled(commandId.toStdString(), state);
      bool checked = commandId == "tab.pin" ? state.is_pinned : false;
      const auto &pinLabel = state.is_pinned ? "Unpin" : "Pin";

      addItem(items, ContextMenuItemKind::Action, commandId,
              QString::fromUtf8(commandId == "tab.pin" ? pinLabel
                                                       : commandSpec.label),
              QString::fromUtf8(commandSpec.shortcut),
              QString::fromUtf8(commandSpec.iconKey), enabled, true, checked);
    }

    return items;
  });
}

#include "command_manager.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/main_window/controllers/app_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <cstdint>
#include <map>

namespace {
enum class TabCommandGroup : uint8_t {
  ClosePrimary = 0,
  CloseSides = 1,
  CloseAllOrClean = 2,
  Pin = 3,
  Path = 4,
};

struct TabCommandSpec {
  const char *id;
  const char *label;
  const char *shortcut;
  const char *iconKey;
  TabCommandGroup group;
};

namespace k {
// TODO(scarlet): Map between Cmd/Meta/Ctrl depending on platform
// NOLINTNEXTLINE
const TabCommandSpec tabCommandSpecs[] = {
    {"tab.close", "Close", "Ctrl+W", "", TabCommandGroup::ClosePrimary},
    {"tab.closeOthers", "Close Others", "", "", TabCommandGroup::ClosePrimary},
    {"tab.closeLeft", "Close Left", "", "", TabCommandGroup::CloseSides},
    {"tab.closeRight", "Close Right", "", "", TabCommandGroup::CloseSides},
    {"tab.closeAll", "Close All", "", "", TabCommandGroup::CloseAllOrClean},
    {"tab.closeClean", "Close Clean", "", "", TabCommandGroup::CloseAllOrClean},
    {"tab.pin", "Pin", "", "", TabCommandGroup::Pin},
    {"tab.copyPath", "Copy Path", "", "", TabCommandGroup::Path},
    {"tab.reveal", "Reveal in Explorer", "Cmd+Shift+E", "",
     TabCommandGroup::Path},
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
      {"tab.close", state.can_close},
      {"tab.closeOthers", state.can_close_others},
      {"tab.closeLeft", state.can_close_left},
      {"tab.closeRight", state.can_close_right},
      {"tab.closeAll", state.can_close_all},
      {"tab.closeClean", state.can_close_clean},
      {"tab.copyPath", state.can_copy_path},
      {"tab.reveal", state.can_reveal},
      {"tab.pin", true},
  };

  return enabledMap[commandId];
};
} // namespace

Q_DECLARE_METATYPE(neko::TabContextFfi);

CommandManager::CommandManager(const CommandManagerProps &props)
    : commandRegistry(props.commandRegistry),
      contextMenuRegistry(props.contextMenuRegistry),
      workspaceCoordinator(props.workspaceCoordinator),
      appController(props.appController) {
  qRegisterMetaType<neko::TabContextFfi>("TabContext");

  registerProviders();
  registerCommands();
}

void CommandManager::registerCommands() {
  // Tab commands
  commandRegistry->registerCommand("tab.close",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.close", variant);
                                   });
  commandRegistry->registerCommand(
      "tab.closeOthers", [this](const QVariant &variant) {
        handleTabCommand("tab.closeOthers", variant);
      });
  commandRegistry->registerCommand("tab.closeLeft",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.closeLeft", variant);
                                   });
  commandRegistry->registerCommand(
      "tab.closeRight", [this](const QVariant &variant) {
        handleTabCommand("tab.closeRight", variant);
      });
  commandRegistry->registerCommand("tab.closeAll",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.closeAll", variant);
                                   });
  commandRegistry->registerCommand(
      "tab.closeClean", [this](const QVariant &variant) {
        handleTabCommand("tab.closeClean", variant);
      });
  commandRegistry->registerCommand("tab.copyPath",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.copyPath", variant);
                                   });
  commandRegistry->registerCommand("tab.reveal",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.reveal", variant);
                                   });
  commandRegistry->registerCommand("tab.copyPath",
                                   [this](const QVariant &variant) {
                                     handleTabCommand("tab.copyPath", variant);
                                   });
  commandRegistry->registerCommand("tab.pin", [this](const QVariant &variant) {
    handleTabCommand("tab.pin", variant);
  });
}

void CommandManager::registerProviders() {
  contextMenuRegistry->registerProvider("tab", [this](const QVariant &variant) {
    auto availableCommands = AppController::getAvailableTabCommands();

    const auto ctx = variant.value<neko::TabContextFfi>();
    const auto state = appController->getTabCommandState(ctx);
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

void CommandManager::handleTabCommand(const QString &commandId,
                                      const QVariant &variant) {
  const auto ctx = variant.value<neko::TabContextFfi>();
  const bool shiftHeld =
      QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

  workspaceCoordinator->handleTabCommand(commandId.toStdString(), ctx,
                                         shiftHeld);
}

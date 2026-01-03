#include "command_manager.h"
#include "core/bridge/app_bridge.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
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

enum class FileExplorerCommandGroup : uint8_t {
  CreateOrOpen = 1,        // New File / New Folder / Reveal / Open in Terminal
  Search = 2,              // Find in Folder
  ClipboardOperations = 3, // Cut / Copy / Paste / Duplicate
  PathOperations = 4,      // Copy Path / Copy Relative Path
  HistoryOrModify = 5,     // Show History / Rename / Delete
  TreeOperations = 7,      // Expand / Collapse / Collapse All
};

template <typename T> struct CommandSpec {
  const char *id;
  const char *label;
  const char *shortcut;
  const char *iconKey;
  T group;
};

namespace k {
// TODO(scarlet): Map between Cmd/Meta/Ctrl depending on platform
// NOLINTNEXTLINE
const CommandSpec<TabCommandGroup> tabCommandSpecs[] = {
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

// NOLINTNEXTLINE
const CommandSpec<FileExplorerCommandGroup> fileExplorerCommandSpecs[] = {
    {"fileExplorer.newFile", "New File", "%", "",
     FileExplorerCommandGroup::CreateOrOpen},
    {"fileExplorer.newFolder", "New Folder", "D", "",
     FileExplorerCommandGroup::CreateOrOpen},
    {"fileExplorer.reveal", "Show on Disk", "X", "",
     FileExplorerCommandGroup::CreateOrOpen},
    {"fileExplorer.openInTerminal", "Open In Terminal", "", "",
     FileExplorerCommandGroup::CreateOrOpen},
    {"fileExplorer.findInFolder", "Find in Folder", "/", "",
     FileExplorerCommandGroup::Search},
    {"fileExplorer.cut", "Cut", "Ctrl+X", "",
     FileExplorerCommandGroup::ClipboardOperations},
    {"fileExplorer.copy", "Copy", "Ctrl+C", "",
     FileExplorerCommandGroup::ClipboardOperations},
    {"fileExplorer.duplicate", "Duplicate", "Ctrl+D", "",
     FileExplorerCommandGroup::ClipboardOperations},
    {"fileExplorer.paste", "Paste", "Ctrl+V", "",
     FileExplorerCommandGroup::ClipboardOperations},
    {"fileExplorer.copyPath", "Copy Path", "Ctrl+Option+C", "",
     FileExplorerCommandGroup::PathOperations},
    {"fileExplorer.copyRelativePath", "Copy Relative Path",
     "Ctrl+Option+Shift+C", "", FileExplorerCommandGroup::PathOperations},
    {"fileExplorer.showHistory", "File History", "", "",
     FileExplorerCommandGroup::HistoryOrModify},
    {"fileExplorer.rename", "Rename", "Shift+R", "",
     FileExplorerCommandGroup::HistoryOrModify},
    {"fileExplorer.delete", "Delete", "Shift+D", "",
     FileExplorerCommandGroup::HistoryOrModify},
    {"fileExplorer.expand", "Expand", "E", "",
     FileExplorerCommandGroup::TreeOperations},
    {"fileExplorer.collapse", "Collapse", "C", "",
     FileExplorerCommandGroup::TreeOperations},
    {"fileExplorer.collapseAll", "Collapse All", "Shift+C", "",
     FileExplorerCommandGroup::TreeOperations},
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
      appBridge(props.appBridge) {
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
    auto availableCommands =
        appBridge->getCommandController()->get_available_tab_commands();

    const auto ctx = variant.value<neko::TabContextFfi>();
    const auto state = appBridge->getTabCommandState(ctx);
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

  contextMenuRegistry->registerProvider(
      "fileExplorer", [this](const QVariant &variant) {
        auto availableCommands = appBridge->getCommandController()
                                     ->get_available_file_explorer_commands();

        const auto ctx = variant.value<neko::FileExplorerContextFfi>();
        const auto state = appBridge->getFileExplorerCommandState(ctx);
        QVector<ContextMenuItem> items;

        QSet<QString> availableIds;
        for (auto &command : availableCommands) {
          availableIds.insert(QString::fromStdString(command.id.c_str()));
        }

        bool firstItem = true;
        std::optional<FileExplorerCommandGroup> lastGroup;

        for (const auto &commandSpec : k::fileExplorerCommandSpecs) {
          const QString commandId = QString::fromUtf8(commandSpec.id);
          if (!availableIds.contains(commandId)) {
            continue;
          }

          // Insert separators when changing groups
          if (!firstItem) {
            if (!lastGroup.has_value() ||
                commandSpec.group != lastGroup.value()) {
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
                  QString::fromUtf8(commandSpec.iconKey), enabled, true,
                  checked);
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

#include "command_manager.h"
#include "core/bridge/app_bridge.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <cstdint>
#include <map>

Q_DECLARE_METATYPE(neko::TabContextFfi);
Q_DECLARE_METATYPE(neko::FileExplorerContextFfi);

namespace {
enum class TabCommandGroup : uint8_t {
  ClosePrimary = 0,
  CloseSides = 1,
  CloseAllOrClean = 2,
  Pin = 3,
  Path = 4,
};

enum class FileExplorerCommandGroup : uint8_t {
  Create = 1,              // New File / New Folder
  Open = 2,                // Reveal / Open in Terminal
  Search = 3,              // Find in Folder
  ClipboardOperations = 4, // Cut / Copy / Paste / Duplicate
  PathOperations = 5,      // Copy Path / Copy Relative Path
  History = 6,             // Show History
  Modify = 7,              // Rename / Delete
  TreeOperations = 8,      // Expand / Collapse / Collapse All
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
     FileExplorerCommandGroup::Create},
    {"fileExplorer.newFolder", "New Folder", "D", "",
     FileExplorerCommandGroup::Create},
    {"fileExplorer.reveal", "Show on Disk", "X", "",
     FileExplorerCommandGroup::Open},
    {"fileExplorer.openInTerminal", "Open in Terminal", "", "",
     FileExplorerCommandGroup::Open},
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
     FileExplorerCommandGroup::History},
    {"fileExplorer.rename", "Rename", "Shift+R", "",
     FileExplorerCommandGroup::Modify},
    {"fileExplorer.delete", "Delete", "Shift+D", "",
     FileExplorerCommandGroup::Modify},
    {"fileExplorer.expand", "Expand", "E", "",
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

inline bool
isFileExplorerCommandEnabled(const std::string &commandId,
                             const neko::FileExplorerCommandStateFfi state) {
  std::map<std::string, bool> enabledMap{
      {"fileExplorer.newFile", state.can_make_new_file},
      {"fileExplorer.newFolder", state.can_make_new_folder},
      {"fileExplorer.reveal", state.can_reveal_in_system},
      {"fileExplorer.openInTerminal", state.can_open_in_terminal},
      {"fileExplorer.findInFolder", state.can_find_in_folder},
      {"fileExplorer.cut", state.can_cut},
      {"fileExplorer.copy", state.can_copy},
      {"fileExplorer.duplicate", state.can_duplicate},
      {"fileExplorer.paste", state.can_paste},
      {"fileExplorer.copyPath", state.can_copy_path},
      {"fileExplorer.copyRelativePath", state.can_copy_relative_path},
      {"fileExplorer.showHistory", state.can_show_history},
      {"fileExplorer.rename", state.can_rename},
      {"fileExplorer.delete", state.can_delete},
      {"fileExplorer.expand", state.can_expand_item},
      {"fileExplorer.collapseAll", state.can_collapse_all},
  };

  return enabledMap[commandId];
};
} // namespace

CommandManager::CommandManager(const CommandManagerProps &props)
    : commandRegistry(props.commandRegistry),
      contextMenuRegistry(props.contextMenuRegistry),
      workspaceCoordinator(props.workspaceCoordinator),
      appBridge(props.appBridge) {
  qRegisterMetaType<neko::TabContextFfi>("TabContext");

  registerProviders();
  registerCommands();
}

// TODO(scarlet): Convert these to a loop?
void CommandManager::registerCommands() {
  // Tab commands
  commandRegistry->registerCommand(
      "tab.close", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.close", variant);
      });
  commandRegistry->registerCommand(
      "tab.closeOthers", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.closeOthers", variant);
      });
  commandRegistry->registerCommand(
      "tab.closeLeft", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.closeLeft", variant);
      });
  commandRegistry->registerCommand(
      "tab.closeRight", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.closeRight", variant);
      });
  commandRegistry->registerCommand(
      "tab.closeAll", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.closeAll", variant);
      });
  commandRegistry->registerCommand(
      "tab.closeClean", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.closeClean", variant);
      });
  commandRegistry->registerCommand(
      "tab.copyPath", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.copyPath", variant);
      });
  commandRegistry->registerCommand(
      "tab.reveal", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.reveal", variant);
      });
  commandRegistry->registerCommand(
      "tab.copyPath", [this](const QVariant &variant) {
        handleCommand(CommandType::Tab, "tab.copyPath", variant);
      });
  commandRegistry->registerCommand("tab.pin", [this](const QVariant &variant) {
    handleCommand(CommandType::Tab, "tab.pin", variant);
  });

  // File Explorer commands
  commandRegistry->registerCommand(
      "fileExplorer.newFile", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.newFile",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.newFolder", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.newFolder",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.reveal", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.reveal",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.openInTerminal", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.openInTerminal",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.findInFolder", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.findInFolder",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.cut", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.cut", variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.copy", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.copy", variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.duplicate", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.duplicate",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.paste", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.paste", variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.copyPath", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.copyPath",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.copyRelativePath", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer,
                      "fileExplorer.copyRelativePath", variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.showHistory", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.showHistory",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.rename", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.rename",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.delete", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.delete",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.expand", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.expand",
                      variant);
      });
  commandRegistry->registerCommand(
      "fileExplorer.collapseAll", [this](const QVariant &variant) {
        handleCommand(CommandType::FileExplorer, "fileExplorer.collapseAll",
                      variant);
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

  contextMenuRegistry->registerProvider("fileExplorer", [this](const QVariant
                                                                   &variant) {
    const auto ctx = variant.value<neko::FileExplorerContextFfi>();
    auto availableCommands =
        appBridge->getCommandController()->get_available_file_explorer_commands(
            ctx);

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
        if (!lastGroup.has_value() || commandSpec.group != lastGroup.value()) {
          addSeparatorIfNotEmpty(items);
        }
      }

      firstItem = false;
      lastGroup = commandSpec.group;

      bool enabled =
          isFileExplorerCommandEnabled(commandId.toStdString(), state);
      bool checked =
          commandId == "fileExplorer.expand" ? ctx.item_is_expanded : false;
      const auto &expandOrCollapseLabel =
          (ctx.item_is_directory && !ctx.item_is_expanded) ? "Expand"
                                                           : "Collapse";

      addItem(items, ContextMenuItemKind::Action, commandId,
              QString::fromUtf8(commandId == "fileExplorer.expand"
                                    ? expandOrCollapseLabel
                                    : commandSpec.label),
              QString::fromUtf8(commandSpec.shortcut),
              QString::fromUtf8(commandSpec.iconKey), enabled, true, checked);
    }

    return items;
  });
}

void CommandManager::handleCommand(const CommandType commandType,
                                   const QString &commandId,
                                   const QVariant &variant) {
  const bool shiftHeld =
      QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

  switch (commandType) {
  case CommandType::Tab: {
    const auto ctx = variant.value<neko::TabContextFfi>();
    workspaceCoordinator->handleCommand<neko::TabContextFfi>(
        commandId.toStdString(), ctx, shiftHeld);
    break;
  }
  case CommandType::FileExplorer: {
    const auto ctx = variant.value<neko::FileExplorerContextFfi>();
    workspaceCoordinator->handleCommand<neko::FileExplorerContextFfi>(
        commandId.toStdString(), ctx, shiftHeld);
    break;
  }
  }
}

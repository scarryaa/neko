#include "shortcuts_manager.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/ui_handles.h"
#include "features/tabs/bridge/tab_bridge.h"
#include "neko-core/src/ffi/bridge.rs.h"

ShortcutsManager::ShortcutsManager(const ShortcutsManagerProps &props,
                                   QObject *parent)
    : actionOwner(props.actionOwner),
      nekoShortcutsManager(props.shortcutsManager),
      workspaceCoordinator(props.workspaceCoordinator),
      tabBridge(props.tabBridge), uiHandles(props.uiHandles), QObject(parent) {
  populateShortcutMetadata();
  setUpKeyboardShortcuts();
}

void ShortcutsManager::setUpKeyboardShortcuts() {
  syncShortcutsFromRust();
  registerAllShortcuts();
}

void ShortcutsManager::populateShortcutMetadata() {
  shortcutActionMap = {
      {
          "Tab::Open",
          [this]() { workspaceCoordinator->openFile(); },
      },
      {
          "Tab::Save",
          [this]() { workspaceCoordinator->fileSaved(false); },
      },
      {
          "Tab::SaveAs",
          [this]() { workspaceCoordinator->fileSaved(true); },
      },
      {
          "Tab::New",
          [this]() { workspaceCoordinator->newTab(); },
      },
      {
          "Tab::Close",
          [this]() {
            const auto snapshot = tabBridge->getTabsSnapshot();
            workspaceCoordinator->closeTabs(
                neko::CloseTabOperationTypeFfi::Single,
                static_cast<int>(snapshot.active_id), false);
          },
      },
      {
          "Tab::ForceClose",
          [this]() {
            const auto snapshot = tabBridge->getTabsSnapshot();
            workspaceCoordinator->closeTabs(
                neko::CloseTabOperationTypeFfi::Single,
                static_cast<int>(snapshot.active_id), true);
          },
      },
      {
          "Tab::NextLinear",
          [this]() { WorkspaceCoordinator::moveTabBy(+1, false); },
      },
      {
          "Tab::PreviousLinear",
          [this]() { WorkspaceCoordinator::moveTabBy(-1, false); },
      },
      {
          "Tab::HistoryForward",
          [this]() { WorkspaceCoordinator::moveTabBy(+1, true); },
      },
      {
          "Tab::HistoryBack",
          [this]() { WorkspaceCoordinator::moveTabBy(-1, true); },
      },
      {
          "Tab::Reveal",
          [this]() { workspaceCoordinator->revealActiveTab(); },
      },
      {
          "Cursor::JumpTo",
          [this]() { workspaceCoordinator->cursorPositionClicked(); },
      },
      {
          "FileExplorer::Toggle",
          [this]() { workspaceCoordinator->fileExplorerToggled(); },
      },
      {
          "FileExplorer::Focus",
          [this]() { uiHandles->fileExplorerWidget->setFocus(); },
      },
      {
          "Editor::Focus",
          [this]() { uiHandles->editorWidget->setFocus(); },
      },
      {
          "Editor::OpenConfig",
          [this]() { workspaceCoordinator->openConfig(); },
      },
      {
          "CommandPalette::Show",
          [this]() {
            uiHandles->commandPaletteWidget->showPalette(
                CommandPaletteMode::Command, {});
          },
      },
  };
}

void ShortcutsManager::syncShortcutsFromRust() {
  auto rustShortcuts = nekoShortcutsManager->get_shortcuts();

  shortcutMap.clear();
  shortcutMap.reserve(rustShortcuts.size());

  for (const auto &shortcut : rustShortcuts) {
    std::string key = static_cast<rust::String>(shortcut.key).c_str();
    std::string keyCombo =
        static_cast<rust::String>(shortcut.key_combo).c_str();
    bool scopeToEditor = key == "Tab::Reveal";

    ShortcutFn action;
    if (auto foundAction = shortcutActionMap.find(key);
        foundAction != shortcutActionMap.end()) {
      action = foundAction->second;
    }

    shortcutMap.emplace(
        key,
        Shortcut{
            // TODO(scarlet): Convert this to a map later if needed
            .owner = scopeToEditor ? uiHandles->editorWidget : actionOwner,
            .key = key,
            .keyCombo = std::move(keyCombo),
            // TODO(scarlet): Convert this to a map later if needed
            .context = scopeToEditor
                           ? Qt::ShortcutContext::WidgetWithChildrenShortcut
                           : Qt::ShortcutContext::WindowShortcut,
            .action = std::move(action),
        });
  }

  rust::Vec<neko::Shortcut> missingShortcuts;

  auto ensureShortcut = [&](const std::string &key,
                            const std::string &keyCombo) {
    auto foundShortcut = shortcutMap.find(key);
    if (foundShortcut != shortcutMap.end() &&
        !foundShortcut->second.key.empty()) {
      // If there is a Rust-provided shortcut for this key already, skip it
      return;
    }

    // Mark the shortcut as missing
    neko::Shortcut shortcut;
    shortcut.key = key;
    shortcut.key_combo = keyCombo;
    missingShortcuts.push_back(std::move(shortcut));

    const bool scopeToEditor = (key == "Tab::Reveal");
    auto *owner = scopeToEditor ? uiHandles->editorWidget : actionOwner;
    auto context = scopeToEditor
                       ? Qt::ShortcutContext::WidgetWithChildrenShortcut
                       : Qt::ShortcutContext::WindowShortcut;

    ShortcutFn action;
    if (auto foundAction = shortcutActionMap.find(key);
        foundAction != shortcutActionMap.end()) {
      action = foundAction->second;
    }

    // Add the missing shortcut to the shortcut map
    shortcutMap[key] = Shortcut{
        .owner = owner,
        .key = key,
        .keyCombo = keyCombo,
        .context = context,
        .action = std::move(action),
    };
  };

  ensureShortcut("Tab::Open", "Ctrl+O");
  ensureShortcut("Tab::Save", "Ctrl+S");
  ensureShortcut("Tab::SaveAs", "Ctrl+Shift+S");
  ensureShortcut("Tab::New", "Ctrl+T");
  ensureShortcut("Tab::Close", "Ctrl+W");
  ensureShortcut("Tab::ForceClose", "Ctrl+Shift+W");
  ensureShortcut("Tab::NextLinear", "Meta+Tab");
  ensureShortcut("Tab::PreviousLinear", "Meta+Shift+Tab");
  ensureShortcut("Tab::HistoryForward", "Meta+]");
  ensureShortcut("Tab::HistoryBack", "Meta+[");
  ensureShortcut("Tab::Reveal", "Ctrl+Shift+E");
  ensureShortcut("Cursor::JumpTo", "Ctrl+G");
  ensureShortcut("FileExplorer::Toggle", "Ctrl+E");
  ensureShortcut("FileExplorer::Focus", "Meta+H");
  ensureShortcut("Editor::Focus", "Meta+L");
  ensureShortcut("Editor::OpenConfig", "Ctrl+,");
  ensureShortcut("CommandPalette::Show", "Ctrl+P");

  if (!missingShortcuts.empty()) {
    nekoShortcutsManager->add_shortcuts(std::move(missingShortcuts));
  }
}

void ShortcutsManager::registerAllShortcuts() {
  for (const auto &shortcutEntry : shortcutMap) {
    const Shortcut &shortcut = shortcutEntry.second;
    const QKeySequence keyCombo(shortcut.keyCombo.c_str());

    if (shortcut.owner != nullptr) {
      registerShortcut(shortcut.owner, shortcut.key, keyCombo, shortcut.context,
                       shortcut.action);
    } else {
      registerShortcut(shortcut.key, keyCombo, shortcut.context,
                       shortcut.action);
    }
  }
}

QKeySequence ShortcutsManager::seqFor(const std::string &key,
                                      const QKeySequence &fallback) const {
  auto foundShortcut = shortcutMap.find(key);
  if (foundShortcut != shortcutMap.end() &&
      !foundShortcut->second.key.empty()) {
    return {foundShortcut->second.keyCombo.c_str()};
  }

  return fallback;
}

template <typename Slot>
void ShortcutsManager::registerShortcut(const std::string &key,
                                        const QKeySequence &fallback,
                                        Qt::ShortcutContext context,
                                        Slot &&slot) {
  auto *action = new QAction(this);
  addShortcut(action, seqFor(key, fallback), context, std::forward<Slot>(slot));
}

template <typename Slot>
void ShortcutsManager::registerShortcut(QWidget *owner, const std::string &key,
                                        const QKeySequence &fallback,
                                        Qt::ShortcutContext context,
                                        Slot &&slot) {
  auto *action = new QAction(owner);
  addShortcut(owner, action, seqFor(key, fallback), context,
              std::forward<Slot>(slot));
}

template <typename Slot>
void ShortcutsManager::addShortcut(QWidget *owner, QAction *action,
                                   const QKeySequence &sequence,
                                   Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  owner->addAction(action);
}

template <typename Slot>
void ShortcutsManager::addShortcut(QAction *action,
                                   const QKeySequence &sequence,
                                   Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  actionOwner->addAction(action);
}

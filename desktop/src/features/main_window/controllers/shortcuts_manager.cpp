#include "shortcuts_manager.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/ui_handles.h"
#include "features/tabs/controllers/tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"

ShortcutsManager::ShortcutsManager(const ShortcutsManagerProps &props,
                                   QObject *parent)
    : actionOwner(props.actionOwner),
      nekoShortcutsManager(props.shortcutsManager),
      workspaceCoordinator(props.workspaceCoordinator),
      tabController(props.tabController), uiHandles(props.uiHandles),
      QObject(parent) {
  setUpKeyboardShortcuts();
}

void ShortcutsManager::setUpKeyboardShortcuts() {
  syncShortcutsFromRust();
  registerAllShortcuts();
}

void ShortcutsManager::syncShortcutsFromRust() {
  auto rustShortcuts = nekoShortcutsManager->get_shortcuts();

  shortcutMap.clear();
  shortcutMap.reserve(rustShortcuts.size());

  for (const auto &shortcut : rustShortcuts) {
    shortcutMap.emplace(
        std::string(static_cast<rust::String>(shortcut.key).c_str()),
        std::string(static_cast<rust::String>(shortcut.key_combo).c_str()));
  }

  rust::Vec<neko::Shortcut> missingShortcuts;

  auto ensureShortcut = [&](const std::string &key,
                            const std::string &keyCombo) {
    auto foundShortcut = shortcutMap.find(key);
    if (foundShortcut == shortcutMap.end() || foundShortcut->second.empty()) {
      neko::Shortcut shortcut;
      shortcut.key = key;
      shortcut.key_combo = keyCombo;
      missingShortcuts.push_back(std::move(shortcut));
      shortcutMap[key] = keyCombo;
    }
  };

  ensureShortcut("Tab::Save", "Ctrl+S");
  ensureShortcut("Tab::SaveAs", "Ctrl+Shift+S");
  ensureShortcut("Tab::New", "Ctrl+T");
  ensureShortcut("Tab::Close", "Ctrl+W");
  ensureShortcut("Tab::ForceClose", "Ctrl+Shift+W");
  ensureShortcut("Tab::Next", "Meta+Tab");
  ensureShortcut("Tab::Previous", "Meta+Shift+Tab");
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
  auto findIndexById = [](const neko::TabsSnapshot &snap,
                          uint64_t tabId) -> int {
    for (int i = 0; i < static_cast<int>(snap.tabs.size()); ++i) {
      if (snap.tabs[i].id == tabId) {
        return i;
      }
    }
    return -1;
  };

  // Save
  registerShortcut("Tab::Save", QKeySequence(Qt::ControlModifier | Qt::Key_S),
                   Qt::WindowShortcut,
                   [this]() { workspaceCoordinator->fileSaved(false); });

  // Save As
  registerShortcut(
      "Tab::SaveAs",
      QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->fileSaved(true); });

  // New Tab
  registerShortcut("Tab::New", QKeySequence(Qt::ControlModifier | Qt::Key_T),
                   Qt::WindowShortcut,
                   [this]() { workspaceCoordinator->newTab(); });

  // Close Tab
  registerShortcut("Tab::Close", QKeySequence(Qt::ControlModifier | Qt::Key_W),
                   Qt::WindowShortcut, [this]() {
                     const auto snapshot = tabController->getTabsSnapshot();
                     workspaceCoordinator->closeTab(
                         static_cast<int>(snapshot.active_id), false);
                   });

  // Force Close
  registerShortcut(
      "Tab::ForceClose",
      QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_W),
      Qt::WindowShortcut, [this]() {
        const auto snapshot = tabController->getTabsSnapshot();
        workspaceCoordinator->closeTab(static_cast<int>(snapshot.active_id),
                                       true);
      });

  // Next Tab
  registerShortcut("Tab::Next", QKeySequence(Qt::MetaModifier | Qt::Key_Tab),
                   Qt::WindowShortcut, [this, findIndexById]() {
                     const auto snapshot = tabController->getTabsSnapshot();
                     if (!snapshot.active_present) {
                       return;
                     }

                     const int tabCount =
                         static_cast<int>(snapshot.tabs.size());
                     if (tabCount <= 1) {
                       return;
                     }

                     const int currentIndex =
                         findIndexById(snapshot, snapshot.active_id);
                     if (currentIndex < 0) {
                       return;
                     }

                     const int nextIndex = (currentIndex + 1) % tabCount;
                     workspaceCoordinator->tabChanged(
                         static_cast<int>(snapshot.tabs[nextIndex].id));
                   });

  // Previous Tab
  registerShortcut(
      "Tab::Previous",
      QKeySequence(Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_Tab),
      Qt::WindowShortcut, [this, findIndexById]() {
        const auto snapshot = tabController->getTabsSnapshot();
        if (!snapshot.active_present) {
          return;
        }

        const int tabCount = static_cast<int>(snapshot.tabs.size());
        if (tabCount <= 1) {
          return;
        }

        const int currentIndex = findIndexById(snapshot, snapshot.active_id);
        if (currentIndex < 0) {
          return;
        }

        const int prevIndex =
            (currentIndex == 0) ? (tabCount - 1) : (currentIndex - 1);
        workspaceCoordinator->tabChanged(
            static_cast<int>(snapshot.tabs[prevIndex].id));
      });

  // Jump To
  registerShortcut("Cursor::JumpTo",
                   QKeySequence(Qt::ControlModifier | Qt::Key_G),
                   Qt::WindowShortcut,
                   [this]() { workspaceCoordinator->cursorPositionClicked(); });

  // Toggle Explorer
  registerShortcut("FileExplorer::Toggle",
                   QKeySequence(Qt::ControlModifier | Qt::Key_E),
                   Qt::WindowShortcut,
                   [this]() { workspaceCoordinator->fileExplorerToggled(); });

  // Focus Explorer
  registerShortcut("FileExplorer::Focus",
                   QKeySequence(Qt::MetaModifier | Qt::Key_H),
                   Qt::WindowShortcut,
                   [this]() { uiHandles->fileExplorerWidget->setFocus(); });

  // Focus Editor
  registerShortcut("Editor::Focus", QKeySequence(Qt::MetaModifier | Qt::Key_L),
                   Qt::WindowShortcut,
                   [this]() { uiHandles->editorWidget->setFocus(); });

  // Open Config
  registerShortcut(
      "Editor::OpenConfig", QKeySequence(Qt::ControlModifier | Qt::Key_Comma),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->openConfig(); });

  // Show Command Palette
  registerShortcut("CommandPalette::Show",
                   QKeySequence(Qt::ControlModifier | Qt::Key_P),
                   Qt::WindowShortcut, [this]() {
                     uiHandles->commandPaletteWidget->showPalette(
                         CommandPaletteMode::Command, {});
                   });
}

QKeySequence ShortcutsManager::seqFor(const std::string &key,
                                      const QKeySequence &fallback) const {
  auto foundShortcut = shortcutMap.find(key);
  if (foundShortcut != shortcutMap.end() && !foundShortcut->second.empty()) {
    return {foundShortcut->second.c_str()};
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
void ShortcutsManager::addShortcut(QAction *action,
                                   const QKeySequence &sequence,
                                   Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  actionOwner->addAction(action);
}

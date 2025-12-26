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
  // TODO(scarlet): Break this down
  auto rustShortcuts = nekoShortcutsManager->get_shortcuts();
  std::unordered_map<std::string, std::string> shortcutMap;
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

  for (const auto &shortcut : rustShortcuts) {
    ensureShortcut(shortcut.key.data(), shortcut.key_combo.data());
  }

  if (!missingShortcuts.empty()) {
    nekoShortcutsManager->add_shortcuts(std::move(missingShortcuts));
  }

  auto seqFor = [&](const std::string &key,
                    const QKeySequence &fallback) -> QKeySequence {
    auto foundShortcut = shortcutMap.find(key);
    if (foundShortcut != shortcutMap.end() && !foundShortcut->second.empty()) {
      return {foundShortcut->second.c_str()};
    }
    return fallback;
  };

  // Cmd+S for save
  auto *saveAction = new QAction(this);
  addShortcut(
      saveAction,
      seqFor("Tab::Save", QKeySequence(Qt::ControlModifier | Qt::Key_S)),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->fileSaved(false); });

  // Cmd+Shift+S for save as
  auto *saveAsAction = new QAction(this);
  addShortcut(
      saveAsAction,
      seqFor("Tab::SaveAs",
             QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S)),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->fileSaved(true); });

  // Cmd+T for new tab
  auto *newTabAction = new QAction(this);
  addShortcut(newTabAction,
              seqFor("Tab::New", QKeySequence(Qt::ControlModifier | Qt::Key_T)),
              Qt::WindowShortcut, [this]() { workspaceCoordinator->newTab(); });

  // Cmd+W for close tab
  auto *closeTabAction = new QAction(this);
  addShortcut(
      closeTabAction,
      seqFor("Tab::Close", QKeySequence(Qt::ControlModifier | Qt::Key_W)),
      Qt::WindowShortcut, [this]() {
        const auto snapshot = tabController->getTabsSnapshot();
        workspaceCoordinator->closeTab(static_cast<int>(snapshot.active_id),
                                       false);
      });

  // Cmd+Shift+W for force close tab (bypass unsaved confirmation)
  auto *forceCloseTabAction = new QAction(this);
  addShortcut(forceCloseTabAction,
              seqFor("Tab::ForceClose",
                     QKeySequence(QKeySequence(Qt::ControlModifier,
                                               Qt::ShiftModifier, Qt::Key_W))),
              Qt::WindowShortcut, [this]() {
                const auto snapshot = tabController->getTabsSnapshot();
                workspaceCoordinator->closeTab(
                    static_cast<int>(snapshot.active_id), true);
              });

  auto findIndexById = [](const neko::TabsSnapshot &snap,
                          uint64_t tabId) -> int {
    for (int i = 0; i < static_cast<int>(snap.tabs.size()); ++i) {
      if (snap.tabs[i].id == tabId) {
        return i;
      }
    }
    return -1;
  };

  // Cmd+Tab for next tab
  auto *nextTabAction = new QAction(this);
  addShortcut(nextTabAction,
              seqFor("Tab::Next", QKeySequence(Qt::MetaModifier | Qt::Key_Tab)),
              Qt::WindowShortcut, [this, findIndexById]() {
                const auto snapshot = tabController->getTabsSnapshot();
                if (!snapshot.active_present) {
                  return;
                }

                const int tabCount = static_cast<int>(snapshot.tabs.size());
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

  // Cmd+Shift+Tab for previous tab
  auto *prevTabAction = new QAction(this);
  addShortcut(
      prevTabAction,
      seqFor("Tab::Previous",
             QKeySequence(Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_Tab)),
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

  // Ctrl+G for jump to
  auto *jumpToAction = new QAction(this);
  addShortcut(
      jumpToAction,
      seqFor("Cursor::JumpTo", QKeySequence(Qt::ControlModifier | Qt::Key_G)),
      Qt::WindowShortcut,
      [this]() { workspaceCoordinator->cursorPositionClicked(); });

  // Ctrl + E for File Explorer toggle
  auto *toggleExplorerAction = new QAction(this);
  addShortcut(toggleExplorerAction,
              seqFor("FileExplorer::Toggle",
                     QKeySequence(Qt::ControlModifier | Qt::Key_E)),
              Qt::WindowShortcut,
              [this]() { workspaceCoordinator->fileExplorerToggled(); });

  // Ctrl + H for focus File Explorer
  // TODO(scarlet): Generalize this, i.e. "focus left widget/pane"
  auto *focusExplorerAction = new QAction(this);
  addShortcut(
      focusExplorerAction,
      seqFor("FileExplorer::Focus", QKeySequence(Qt::MetaModifier | Qt::Key_H)),
      Qt::WindowShortcut,
      [this]() { uiHandles->fileExplorerWidget->setFocus(); });

  // Ctrl + L for focus Editor
  // TODO(scarlet): Generalize this, i.e. "focus right widget/pane"
  auto *focusEditorAction = new QAction(this);
  addShortcut(
      focusEditorAction,
      seqFor("Editor::Focus", QKeySequence(Qt::MetaModifier | Qt::Key_L)),
      Qt::WindowShortcut, [this]() { uiHandles->editorWidget->setFocus(); });

  // Ctrl + , for open config
  auto *openConfigAction = new QAction(this);
  addShortcut(openConfigAction,
              seqFor("Editor::OpenConfig",
                     QKeySequence(Qt::ControlModifier | Qt::Key_Comma)),
              Qt::WindowShortcut,
              [this]() { workspaceCoordinator->openConfig(); });

  // Ctrl + P for show command palette
  auto *showCommandPalette = new QAction(this);
  addShortcut(showCommandPalette,
              seqFor("CommandPalette::Show",
                     QKeySequence(Qt::ControlModifier | Qt::Key_P)),
              Qt::WindowShortcut, [this]() {
                uiHandles->commandPaletteWidget->showPalette(
                    CommandPaletteMode::Command, {});
              });
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

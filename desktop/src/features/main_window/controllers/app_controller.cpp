#include "app_controller.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppController::AppController(const AppControllerProps &props)
    : appState(props.appState), appController(neko::new_app_controller(
                                    props.configManager, props.rootPath)) {}

// TabCoreAPI
neko::TabsSnapshot AppController::getTabsSnapshot() {
  return appState->get_tabs_snapshot();
}

std::vector<int>
AppController::getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                              int anchorTabId, bool closePinned) {
  const auto result =
      appState->get_close_tab_ids(operationType, anchorTabId, closePinned);
  return {result.begin(), result.end()};
}

neko::CreateDocumentTabAndViewResultFfi AppController::createDocumentTabAndView(
    const std::string &title, bool addTabToHistory, bool activateView) {
  return appState->create_document_tab_and_view(title, addTabToHistory,
                                                activateView);
}

neko::MoveActiveTabResult AppController::moveTabBy(neko::Buffer buffer,
                                                   int delta, bool useHistory) {
  return appState->move_active_tab_by(buffer, delta, useHistory);
}

bool AppController::moveTab(int fromIndex, int toIndex) {
  return appState->move_tab(fromIndex, toIndex);
}

neko::PinTabResult AppController::pinTab(int tabId) {
  return appState->pin_tab(tabId);
}

neko::PinTabResult AppController::unpinTab(int tabId) {
  return appState->unpin_tab(tabId);
}

neko::CloseManyTabsResult
AppController::closeTabs(neko::CloseTabOperationTypeFfi operationType,
                         int anchorTabId, bool closePinned) {
  return appState->close_tabs(operationType, anchorTabId, closePinned);
}

neko::TabSnapshotMaybe AppController::getTabSnapshot(int tabId) {
  return appState->get_tab_snapshot(tabId);
}

void AppController::setActiveTab(int tabId) { appState->set_active_tab(tabId); }

void AppController::setTabScrollOffsets(int tabId,
                                        const neko::ScrollOffsetFfi &offsets) {
  appState->set_tab_scroll_offsets(tabId, offsets);
}
// End TabCoreAPI

uint64_t AppController::openFile(const std::string &path, bool addToHistory) {
  return appState->ensure_tab_for_path(path, addToHistory);
}

rust::Box<neko::EditorHandle> AppController::getActiveEditorMut() const {
  return appController->get_active_editor_mut();
}

neko::FileTree &AppController::getFileTreeMut() const {
  return appState->get_file_tree_mut();
}

neko::TabCommandStateFfi
AppController::getTabCommandState(const neko::TabContextFfi &ctx) const {
  return neko::get_tab_command_state(*appState, ctx.id);
}

std::vector<neko::CommandFfi> AppController::getAvailableCommands() {
  auto rawCommands = neko::get_available_commands();
  std::vector<neko::CommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

std::vector<neko::JumpCommandFfi> AppController::getAvailableJumpCommands() {
  auto rawCommands = neko::get_available_jump_commands();
  std::vector<neko::JumpCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppController::executeJumpCommand(
    const neko::JumpCommandFfi &jumpCommand) {
  neko::execute_jump_command(jumpCommand, *appState);
}

void AppController::executeJumpKey(const QString &key) {
  neko::execute_jump_key(key.toStdString(), *appState);
}

std::vector<neko::TabCommandFfi> AppController::getAvailableTabCommands() {
  auto rawCommands = neko::get_available_tab_commands();
  std::vector<neko::TabCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppController::runTabCommand(const std::string &commandId,
                                  const neko::TabContextFfi &ctx,
                                  bool closePinned) {
  neko::run_tab_command(*appState, commandId, ctx, closePinned);
}

bool AppController::saveDocument(int documentId) {
  return appState->save_document(documentId);
}

bool AppController::saveDocumentAs(int documentId, const std::string &path) {
  return appState->save_document_as(documentId, path);
}

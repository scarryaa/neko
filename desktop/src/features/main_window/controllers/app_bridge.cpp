#include "app_bridge.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppBridge::AppBridge(const AppBridgeProps &props)
    : appController(
          neko::new_app_controller(props.configManager, props.rootPath)) {}

// TabCoreAPI
// End TabCoreAPI

uint64_t AppBridge::openFile(const std::string &path, bool addToHistory) {
  return appState->ensure_tab_for_path(path, addToHistory);
}

rust::Box<neko::EditorController> AppBridge::getActiveEditorMut() const {
  return appController->editor_controller();
}

rust::Box<neko::TabController> AppBridge::getTabController() const {
  return appController->tab_controller();
}

neko::FileTree &AppBridge::getFileTreeMut() const {
  return appState->get_file_tree_mut();
}

neko::TabCommandStateFfi
AppBridge::getTabCommandState(const neko::TabContextFfi &ctx) const {
  return neko::get_tab_command_state(*appState, ctx.id);
}

std::vector<neko::CommandFfi> AppBridge::getAvailableCommands() {
  auto rawCommands = neko::get_available_commands();
  std::vector<neko::CommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

std::vector<neko::JumpCommandFfi> AppBridge::getAvailableJumpCommands() {
  auto rawCommands = neko::get_available_jump_commands();
  std::vector<neko::JumpCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppBridge::executeJumpCommand(const neko::JumpCommandFfi &jumpCommand) {
  neko::execute_jump_command(jumpCommand, *appState);
}

void AppBridge::executeJumpKey(const QString &key) {
  neko::execute_jump_key(key.toStdString(), *appState);
}

std::vector<neko::TabCommandFfi> AppBridge::getAvailableTabCommands() {
  auto rawCommands = neko::get_available_tab_commands();
  std::vector<neko::TabCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppBridge::runTabCommand(const std::string &commandId,
                              const neko::TabContextFfi &ctx,
                              bool closePinned) {
  neko::run_tab_command(*appState, commandId, ctx, closePinned);
}

bool AppBridge::saveDocument(int documentId) {
  return appState->save_document(documentId);
}

bool AppBridge::saveDocumentAs(int documentId, const std::string &path) {
  return appState->save_document_as(documentId, path);
}

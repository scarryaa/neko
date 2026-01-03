#include "app_bridge.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppBridge::AppBridge(const AppBridgeProps &props)
    : appController(
          neko::new_app_controller(props.configManager, props.rootPath)),
      commandController(appController->command_controller()) {}

neko::OpenTabResultFfi AppBridge::openFile(const std::string &path,
                                           bool addToHistory) {
  return appController->ensure_tab_for_path(path, addToHistory);
}

rust::Box<neko::EditorController> AppBridge::getEditorController() const {
  return appController->editor_controller();
}

rust::Box<neko::TabController> AppBridge::getTabController() const {
  return appController->tab_controller();
}

rust::Box<neko::FileTreeController> AppBridge::getFileTreeController() {
  return appController->file_tree_controller();
}

neko::TabCommandStateFfi
AppBridge::getTabCommandState(const neko::TabContextFfi &ctx) const {
  return commandController->get_tab_command_state(ctx.id);
}

neko::FileExplorerCommandStateFfi AppBridge::getFileExplorerCommandState(
    const neko::FileExplorerContextFfi &ctx) const {
  return commandController->get_file_explorer_command_state(ctx.id);
}

std::vector<neko::CommandFfi> AppBridge::getAvailableCommands() {
  auto commands = commandController->get_available_commands();
  return {commands.begin(), commands.end()};
}

std::vector<neko::JumpCommandFfi> AppBridge::getAvailableJumpCommands() {
  auto commands = commandController->get_available_jump_commands();
  return {commands.begin(), commands.end()};
}

void AppBridge::executeJumpCommand(const neko::JumpCommandFfi &jumpCommand) {
  commandController->execute_jump_command(jumpCommand);
}

void AppBridge::executeJumpKey(const QString &key) {
  commandController->execute_jump_key(key.toStdString());
}

std::vector<neko::TabCommandFfi> AppBridge::getAvailableTabCommands() {
  auto commands = commandController->get_available_tab_commands();
  return {commands.begin(), commands.end()};
}

void AppBridge::runTabCommand(const std::string &commandId,
                              const neko::TabContextFfi &ctx,
                              bool closePinned) {
  commandController->run_tab_command(commandId, ctx, closePinned);
}

bool AppBridge::saveDocument(uint64_t documentId) {
  return appController->save_document(documentId);
}

bool AppBridge::saveDocumentAs(uint64_t documentId, const std::string &path) {
  return appController->save_document_as(documentId, path);
}

neko::CommandController *AppBridge::getCommandController() {
  return &*commandController;
}

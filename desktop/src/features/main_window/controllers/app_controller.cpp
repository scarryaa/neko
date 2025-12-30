#include "app_controller.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppController::AppController(const AppControllerProps &props)
    : appState(props.appState) {}

neko::FileOpenResult AppController::openFile(const std::string &path) {
  return appState->open_file(path);
}

neko::Editor &AppController::getActiveEditorMut() const {
  return appState->get_active_editor_mut();
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

bool AppController::saveTab(int tabId) { return appState->save_tab(tabId); }

bool AppController::saveTabAs(int tabId, const std::string &path) {
  return appState->save_tab_as(tabId, path);
}

neko::FileOpenResult AppController::openFile(int tabId,
                                             const std::string &path) {
  auto openResult = appState->open_file(path);

  if (!openResult.success) {
    // Roll back the added tab
    appState->close_tabs(neko::CloseTabOperationTypeFfi::Single, tabId, false);
    return openResult;
  }

  return openResult;
}

#include "app_state_controller.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppStateController::AppStateController(const AppStateControllerProps &props)
    : appState(props.appState) {}

neko::FileOpenResult AppStateController::openFile(const std::string &path) {
  return appState->open_file(path);
}

neko::Editor &AppStateController::getActiveEditorMut() const {
  return appState->get_active_editor_mut();
}

neko::FileTree &AppStateController::getFileTreeMut() const {
  return appState->get_file_tree_mut();
}

neko::TabCommandStateFfi
AppStateController::getTabCommandState(const neko::TabContextFfi &ctx) const {
  return neko::get_tab_command_state(*appState, ctx.id);
}

std::vector<neko::CommandFfi> AppStateController::getAvailableCommands() {
  auto rawCommands = neko::get_available_commands();
  std::vector<neko::CommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

std::vector<neko::JumpCommandFfi>
AppStateController::getAvailableJumpCommands() {
  auto rawCommands = neko::get_available_jump_commands();
  std::vector<neko::JumpCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppStateController::executeJumpCommand(
    const neko::JumpCommandFfi &jumpCommand) {
  neko::execute_jump_command(jumpCommand, *appState);
}

void AppStateController::executeJumpKey(const QString &key) {
  neko::execute_jump_key(key.toStdString(), *appState);
}

std::vector<neko::TabCommandFfi> AppStateController::getAvailableTabCommands() {
  auto rawCommands = neko::get_available_tab_commands();
  std::vector<neko::TabCommandFfi> commands = {};
  commands.reserve(commands.size());

  for (const auto &command : rawCommands) {
    commands.push_back(command);
  }

  return commands;
}

void AppStateController::runTabCommand(const std::string &commandId,
                                       const neko::TabContextFfi &ctx,
                                       bool closePinned) {
  neko::run_tab_command(*appState, commandId, ctx, closePinned);
}

bool AppStateController::saveTab(int tabId) {
  return appState->save_tab(tabId);
}

bool AppStateController::saveTabAs(int tabId, const std::string &path) {
  return appState->save_tab_as(tabId, path);
}

neko::FileOpenResult AppStateController::openFile(int tabId,
                                                  const std::string &path) {
  auto openResult = appState->open_file(path);

  if (!openResult.success) {
    // Roll back the added tab
    appState->close_tabs(neko::CloseTabOperationTypeFfi::Single, tabId, false);
    return openResult;
  }

  return openResult;
}

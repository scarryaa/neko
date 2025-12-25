#include "app_state_controller.h"
#include <neko-core/src/ffi/bridge.rs.h>

AppStateController::AppStateController(neko::AppState *appState)
    : appState(appState) {}

bool AppStateController::openFile(const std::string &path) {
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

void AppStateController::runTabCommand(const std::string &commandId,
                                       const neko::TabContextFfi &ctx) {
  neko::run_tab_command(*appState, commandId, ctx);
}

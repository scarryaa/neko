#include "features/main_window/controllers/app_state_controller.h"

AppStateController::AppStateController(neko::AppState *appState)
    : appState(appState) {}

AppStateController::~AppStateController() {}

const bool AppStateController::openFile(const std::string &path) {
  return appState->open_file(path);
}

neko::Editor &AppStateController::getActiveEditorMut() const {
  return appState->get_active_editor_mut();
}

neko::FileTree &AppStateController::getFileTreeMut() const {
  return appState->get_file_tree_mut();
}

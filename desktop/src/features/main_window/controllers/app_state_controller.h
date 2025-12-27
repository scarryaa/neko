#ifndef APP_STATE_CONTROLLER_H
#define APP_STATE_CONTROLLER_H

#include "types/ffi_types_fwd.h"
#include <QObject>
#include <vector>

class AppStateController : public QObject {
  Q_OBJECT

public:
  struct AppStateControllerProps {
    neko::AppState *appState;
  };

  explicit AppStateController(const AppStateControllerProps &props);
  ~AppStateController() override = default;

  neko::FileOpenResult openFile(const std::string &path);
  [[nodiscard]] neko::Editor &getActiveEditorMut() const;
  [[nodiscard]] neko::FileTree &getFileTreeMut() const;
  [[nodiscard]] neko::TabCommandStateFfi
  getTabCommandState(const neko::TabContextFfi &ctx) const;
  [[nodiscard]] std::vector<
      neko::TabCommandFfi> static getAvailableTabCommands();

  void runTabCommand(const std::string &commandId,
                     const neko::TabContextFfi &ctx);

private:
  neko::AppState *appState;
};

#endif

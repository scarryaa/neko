#ifndef APP_STATE_CONTROLLER_H
#define APP_STATE_CONTROLLER_H

#include "types/ffi_types_fwd.h"
#include <QObject>

class AppStateController : public QObject {
  Q_OBJECT

public:
  explicit AppStateController(neko::AppState *appState);
  ~AppStateController() override = default;

  bool openFile(const std::string &path);
  [[nodiscard]] neko::Editor &getActiveEditorMut() const;
  [[nodiscard]] neko::FileTree &getFileTreeMut() const;

private:
  neko::AppState *appState;
};

#endif

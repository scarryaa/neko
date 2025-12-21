#ifndef APP_STATE_CONTROLLER_H
#define APP_STATE_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/mod.rs.h>

class AppStateController : public QObject {
  Q_OBJECT

public:
  AppStateController(neko::AppState *appState);
  ~AppStateController();

  const bool openFile(const std::string &path);
  neko::Editor &getActiveEditorMut() const;
  neko::FileTree &getFileTreeMut() const;

private:
  neko::AppState *appState;
};

#endif

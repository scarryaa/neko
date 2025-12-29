#ifndef APP_STATE_CONTROLLER_H
#define APP_STATE_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

class AppStateController : public QObject {
  Q_OBJECT

public:
  struct AppStateControllerProps {
    neko::AppState *appState;
  };

  struct JumpCommandArgs {
    rust::String key;
    rust::String displayName;
    neko::JumpCommandKindFfi kind;
    rust::String argument;
    uint32_t row;
    uint32_t column;
    neko::DocumentTargetFfi documentTarget;
    neko::LineTargetFfi lineTarget;
  };

  explicit AppStateController(const AppStateControllerProps &props);
  ~AppStateController() override = default;

  neko::FileOpenResult openFile(const std::string &path);
  [[nodiscard]] neko::Editor &getActiveEditorMut() const;
  [[nodiscard]] neko::FileTree &getFileTreeMut() const;
  [[nodiscard]] neko::TabCommandStateFfi
  getTabCommandState(const neko::TabContextFfi &ctx) const;
  static std::vector<neko::TabCommandFfi> getAvailableTabCommands();
  static std::vector<neko::CommandFfi> getAvailableCommands();
  static std::vector<neko::JumpCommandFfi> getAvailableJumpCommands();

  void executeJumpCommand(const neko::JumpCommandFfi &jumpCommand);
  void executeJumpKey(const QString &key);

  void runTabCommand(const std::string &commandId,
                     const neko::TabContextFfi &ctx);

  bool saveTab(int tabId);
  bool saveTabAs(int tabId, const std::string &path);

private:
  neko::AppState *appState;
};

#endif

#ifndef APP_STATE_CONTROLLER_H
#define APP_STATE_CONTROLLER_H

#include "core/api/tab_core_api.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

class AppController : public QObject, public ITabCoreApi {
  Q_OBJECT

public:
  struct AppControllerProps {
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

  explicit AppController(const AppControllerProps &props);
  ~AppController() override = default;

  // ITabCoreApi
  neko::TabsSnapshot getTabsSnapshot() override;
  std::vector<int> getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                                  int anchorTabId, bool closePinned) override;
  neko::NewTabResult newTab() override;
  neko::MoveActiveTabResult moveTabBy(int delta, bool useHistory) override;
  bool moveTab(int fromIndex, int toIndex) override;
  neko::PinTabResult pinTab(int tabId) override;
  neko::PinTabResult unpinTab(int tabId) override;
  neko::CloseManyTabsResult
  closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
            bool closePinned) override;
  neko::TabSnapshotMaybe getTabSnapshot(int tabId) override;
  void setActiveTab(int tabId) override;
  void setTabScrollOffsets(int tabId,
                           const neko::ScrollOffsetFfi &offsets) override;
  // End ITabCoreApi

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
                     const neko::TabContextFfi &ctx, bool closePinned);

  bool saveTab(int tabId);
  bool saveTabAs(int tabId, const std::string &path);
  neko::FileOpenResult openFile(int tabId, const std::string &path);

private:
  neko::AppState *appState;
};

#endif

#ifndef APP_BRIDGE_H
#define APP_BRIDGE_H

#include "core/api/tab_core_api.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

class AppBridge : public QObject, public ITabCoreApi {
  Q_OBJECT

public:
  struct AppBridgeProps {
    neko::ConfigManager &configManager;
    const std::string &rootPath;
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

  explicit AppBridge(const AppBridgeProps &props);
  ~AppBridge() override = default;

  // AppController (Rust)
  uint64_t openFile(const std::string &path, bool addToHistory);

  // ITabCoreApi
  neko::TabsSnapshot getTabsSnapshot() override;
  std::vector<int> getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                                  int anchorTabId, bool closePinned) override;
  neko::MoveActiveTabResult moveTabBy(neko::Buffer buffer, int delta,
                                      bool useHistory) override;
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
  neko::CreateDocumentTabAndViewResultFfi
  createDocumentTabAndView(const std::string &title, bool addTabToHistory,
                           bool activateView) override;
  // End ITabCoreApi

  [[nodiscard]] rust::Box<neko::EditorController> getActiveEditorMut() const;
  [[nodiscard]] rust::Box<neko::TabController> getTabController() const;
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

  bool saveDocument(int documentId);
  bool saveDocumentAs(int documentId, const std::string &path);

private:
  rust::Box<neko::AppController> appController;
};

#endif

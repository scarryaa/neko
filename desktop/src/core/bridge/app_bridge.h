#ifndef APP_BRIDGE_H
#define APP_BRIDGE_H

#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

class AppBridge : public QObject {
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

  neko::TabsSnapshot getTabsSnapshot();
  std::vector<int> getCloseTabIds(neko::CloseTabOperationTypeFfi operationType,
                                  int anchorTabId, bool closePinned);
  neko::MoveActiveTabResult moveTabBy(neko::Buffer buffer, int delta,
                                      bool useHistory);
  uint64_t openFile(const std::string &path, bool addToHistory);
  bool moveTab(int fromIndex, int toIndex);
  neko::PinTabResult pinTab(int tabId);
  neko::PinTabResult unpinTab(int tabId);
  neko::CloseManyTabsResult
  closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
            bool closePinned);
  neko::TabSnapshotMaybe getTabSnapshot(int tabId);
  void setActiveTab(int tabId);
  void setTabScrollOffsets(int tabId, const neko::ScrollOffsetFfi &offsets);
  neko::CreateDocumentTabAndViewResultFfi
  createDocumentTabAndView(const std::string &title, bool addTabToHistory,
                           bool activateView);

  [[nodiscard]] rust::Box<neko::EditorController> getEditorController() const;
  [[nodiscard]] rust::Box<neko::TabController> getTabController() const;
  [[nodiscard]] rust::Box<neko::FileTreeController> getFileTreeController();
  [[nodiscard]] neko::TabCommandStateFfi
  getTabCommandState(const neko::TabContextFfi &ctx) const;
  std::vector<neko::TabCommandFfi> getAvailableTabCommands();
  std::vector<neko::CommandFfi> getAvailableCommands();
  std::vector<neko::JumpCommandFfi> getAvailableJumpCommands();

  void executeJumpCommand(const neko::JumpCommandFfi &jumpCommand);
  void executeJumpKey(const QString &key);

  void runTabCommand(const std::string &commandId,
                     const neko::TabContextFfi &ctx, bool closePinned);

  bool saveDocument(uint64_t documentId);
  bool saveDocumentAs(uint64_t documentId, const std::string &path);

  [[nodiscard]] neko::CommandController *getCommandController();

private:
  rust::Box<neko::AppController> appController;
  rust::Box<neko::CommandController> commandController;
};

#endif

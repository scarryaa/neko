#ifndef APP_BRIDGE_H
#define APP_BRIDGE_H

#include "types/command_type.h"
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
  neko::OpenTabResultFfi openFile(const std::string &path, bool addToHistory);
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
  [[nodiscard]] neko::FileExplorerCommandStateFfi
  getFileExplorerCommandState(const neko::FileExplorerContextFfi &ctx) const;
  std::vector<neko::TabCommandFfi> getAvailableTabCommands();
  std::vector<neko::CommandFfi> getAvailableCommands();
  std::vector<neko::JumpCommandFfi> getAvailableJumpCommands();

  void executeJumpCommand(const neko::JumpCommandFfi &jumpCommand);
  void executeJumpKey(const QString &key);

  template <typename Result, typename Context, typename... AdditionalArgs>
  Result runCommand(const CommandType &commandType,
                    const std::string &commandId, const Context &ctx,
                    AdditionalArgs &&...additionalArgs) {
    if constexpr (std::is_same_v<Context, neko::TabContextFfi>) {
      return commandController->run_tab_command(
          commandId, ctx, std::forward<AdditionalArgs>(additionalArgs)...);
    } else if constexpr (std::is_same_v<Context,
                                        neko::FileExplorerContextFfi>) {
      return commandController->run_file_explorer_command(
          commandId, ctx, std::forward<AdditionalArgs>(additionalArgs)...);
    } else {
      static_assert(sizeof(Context) == 0, "Unsupported Context type");
    }
  }

  bool saveDocument(uint64_t documentId);
  bool saveDocumentAs(uint64_t documentId, const std::string &path);

  [[nodiscard]] neko::CommandController *getCommandController();

private:
  rust::Box<neko::AppController> appController;
  rust::Box<neko::CommandController> commandController;
};

#endif

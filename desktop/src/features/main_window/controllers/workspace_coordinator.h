#ifndef WORKSPACE_COORDINATOR_H
#define WORKSPACE_COORDINATOR_H

#include "features/command_palette/types/types.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/flows/file_explorer_flows.h"
#include "features/main_window/flows/tab_flows.h"
#include "features/main_window/interfaces/save_result.h"
#include "features/main_window/ui_handles.h"
#include <QList>
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>
#include <optional>
#include <string>
#include <vector>

class TabBridge;
class AppBridge;
class EditorBridge;
class AppConfigService;
class CommandExecutor;

/// \class WorkspaceCoordinator
/// \brief Wires together controllers, flows, and widgets.
class WorkspaceCoordinator : public QObject {
  Q_OBJECT

public:
  struct WorkspaceCoordinatorProps {
    TabBridge *tabBridge;
    AppBridge *appBridge;
    FileTreeBridge *fileTreeBridge;
    EditorBridge *editorBridge;
    AppConfigService *appConfigService;
    CommandExecutor *commandExecutor;
    const UiHandles uiHandles;
  };

  explicit WorkspaceCoordinator(const WorkspaceCoordinatorProps &props,
                                QObject *parent = nullptr);
  ~WorkspaceCoordinator() override = default;

  template <typename Context>
  void handleCommand(const std::string &commandId, const Context &ctx,
                     bool forceClose) {
    if constexpr (std::is_same_v<Context, neko::TabContextFfi>) {
      const auto succeeded =
          tabFlows.handleTabCommand(commandId, ctx, forceClose);

      // TODO(scarlet): Figure out a better flow/handling for this and avoid
      // matching on raw tab.reveal string.
      if (succeeded && commandId == "tab.reveal") {
        if (uiHandles.fileExplorerWidget->isHidden()) {
          fileExplorerToggled();
        }

        emit tabRevealedInFileExplorer();
      }
    } else if constexpr (std::is_same_v<Context,
                                        neko::FileExplorerContextFfi>) {
      const auto result = fileExplorerFlows.handleFileExplorerCommand(
          commandId, ctx, forceClose);

      // Handle `shouldRedraw` result from Qt-handled commands.
      if (result.success && result.shouldRedraw) {
        emit requestFileExplorerRedraw();
      }

      // Handle 'should redraw' intents from Rust-handled commands.
      for (const auto &intent : result.intentKinds) {
        switch (intent) {
        case neko::FileExplorerUiIntentKindFfi::DirectoryRefreshed:
          emit requestFileExplorerRedraw();
        }
      }
    } else {
      static_assert(sizeof(Context) == 0,
                    "Unsupported Context type in handleCommand");
    }
  }

  // Tab commands & navigation
  void closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool forceClose);
  void newTab();
  void revealActiveTab();
  void moveTabBy(int delta, bool useHistory);

  // Save / close flows
  SaveResult saveTab(int tabId, bool isSaveAs);
  void fileSaved(bool saveAs);

  // File / config actions
  void openFile();
  void applyInitialState();
  void openConfig();
  void fileSelected(const std::string &path, bool focusEditor);

  // Utilities
  [[nodiscard]] std::optional<std::string> requestFileExplorerDirectory() const;
  static std::vector<ShortcutHintRow> buildJumpHintRows(AppBridge *appBridge);

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  // Buffer / tab change events
  void bufferChanged();
  void tabChanged(int tabId);
  void tabUnpinned(int tabId);

  // View toggles & palette interactions
  void fileExplorerToggled();
  void cursorPositionClicked();
  void commandPaletteGoToPosition(const QString &jumpCommandKey, int64_t row,
                                  int64_t column, bool isPosition);
  void commandPaletteCommand(const QString &key, const QString &fullText);

signals:
  void onFileExplorerToggledViaShortcut(bool isOpen);
  void themeChanged(const std::string &themeName);
  void tabRevealedInFileExplorer();
  void fileOpened(const neko::TabSnapshot &tabSnapshot);
  void requestFileExplorerRedraw();

private:
  // Helpers
  void copyTabPath(int tabId);
  void tabTogglePin(int tabId, bool tabIsPinned);
  void refreshUiForActiveTab(bool focusEditor);
  void setEditorController(rust::Box<neko::EditorController> editorController);
  void refreshStatusBarCursorInfo();
  void performFileOpen(const std::string &path);
  [[nodiscard]] QString getInitialDialogDirectory() const;

  TabFlows tabFlows;
  FileExplorerFlows fileExplorerFlows;

  TabBridge *tabBridge;
  AppBridge *appBridge;
  AppConfigService *appConfigService;
  EditorBridge *editorBridge;
  CommandExecutor *commandExecutor;
  const UiHandles uiHandles;
};

#endif // WORKSPACE_COORDINATOR_H

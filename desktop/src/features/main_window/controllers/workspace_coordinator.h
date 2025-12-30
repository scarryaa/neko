#ifndef WORKSPACE_COORDINATOR_H
#define WORKSPACE_COORDINATOR_H

#include "features/command_palette/types/types.h"
#include "features/main_window/flows/tab_flows.h"
#include "features/main_window/interfaces/save_result.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/ui_handles.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <QObject>
#include <optional>
#include <string>
#include <vector>

class TabController;
class AppStateController;
class EditorController;
class AppConfigService;
class CommandExecutor;

/// \class WorkspaceCoordinator
/// \brief Wires together controllers, flows, and widgets.
class WorkspaceCoordinator : public QObject {
  Q_OBJECT

public:
  struct WorkspaceCoordinatorProps {
    TabController *tabController;
    AppStateController *appStateController;
    EditorController *editorController;
    AppConfigService *appConfigService;
    CommandExecutor *commandExecutor;
    const UiHandles uiHandles;
    DialogService *dialogService;
  };

  explicit WorkspaceCoordinator(const WorkspaceCoordinatorProps &props,
                                QObject *parent = nullptr);
  ~WorkspaceCoordinator() override = default;

  // Tab commands & navigation
  void handleTabCommand(const std::string &commandId,
                        const neko::TabContextFfi &ctx, bool forceClose);
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
  [[nodiscard]] static std::vector<ShortcutHintRow> buildJumpHintRows();

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

private:
  // Helpers
  void copyTabPath(int tabId);
  void tabTogglePin(int tabId, bool tabIsPinned);
  void refreshUiForActiveTab(bool focusEditor);
  void setActiveEditor(neko::Editor *editor);
  void refreshStatusBarCursorInfo();

  TabFlows tabFlows;
  DialogService *dialogService;

  TabController *tabController;
  AppStateController *appStateController;
  AppConfigService *appConfigService;
  EditorController *editorController;
  CommandExecutor *commandExecutor;
  const UiHandles uiHandles;
};

#endif // WORKSPACE_COORDINATOR_H

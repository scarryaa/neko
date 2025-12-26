#ifndef WORKSPACE_COORDINATOR_H
#define WORKSPACE_COORDINATOR_H

class TabController;
class WorkspaceController;
class AppStateController;
class UiHandles;
class EditorController;
class AppConfigService;
class CommandExecutor;

#include "features/main_window/interfaces/close_decision.h"
#include "features/main_window/interfaces/save_result.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <QObject>

class WorkspaceCoordinator : public QObject {
  Q_OBJECT

public:
  struct WorkspaceCoordinatorProps {
    WorkspaceController *workspaceController;
    TabController *tabController;
    AppStateController *appStateController;
    EditorController *editorController;
    AppConfigService *appConfigService;
    CommandExecutor *commandExecutor;
    const UiHandles *uiHandles;
  };

  explicit WorkspaceCoordinator(const WorkspaceCoordinatorProps &props,
                                QObject *parent = nullptr);
  ~WorkspaceCoordinator() override = default;

  void handleTabCommand(const std::string &commandId,
                        const neko::TabContextFfi &ctx, bool forceClose);
  CloseDecision showTabCloseConfirmationDialog(const QList<int> &ids);
  SaveResult saveTab(int tabId, bool isSaveAs);

  std::optional<std::string> requestFileExplorerDirectory();
  void fileSaved(bool saveAs);

  void applyInitialState();

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void bufferChanged();

  void fileExplorerToggled();
  void fileSelected(const std::string &path, bool focusEditor);
  void openConfig();

  void cursorPositionClicked();
  void commandPaletteGoToPosition(int row, int col);
  void commandPaletteCommand(const QString &command);

  void newTab();
  void closeTab(int tabId, bool forceClose);
  void tabChanged(int tabId);
  void tabUnpinned(int tabId);

signals:
  void onFileExplorerToggledViaShortcut(bool isOpen);
  void themeChanged(const std::string &themeName);
  void tabRevealedInFileExplorer();

private:
  void tabCopyPath(int tabId);
  void tabTogglePin(int tabId, bool tabIsPinned);
  void tabReveal(const std::string &commandId, const neko::TabContextFfi &ctx);
  void closeLeftTabs(int tabId, bool forceClose);
  void closeRightTabs(int tabId, bool forceClose);
  void closeOtherTabs(int tabId, bool forceClose);
  void closeAllTabs(bool forceClose);
  void closeCleanTabs();

  void saveScrollOffsetsForActiveTab();
  void restoreScrollOffsetsForActiveTab();
  void refreshUiForActiveTab(bool focusEditor);
  void updateTabBar();
  void handleTabsClosed();
  void setActiveEditor(neko::Editor *editor);
  void refreshStatusBarCursorInfo();

  WorkspaceController *workspaceController;
  TabController *tabController;
  AppStateController *appStateController;
  AppConfigService *appConfigService;
  EditorController *editorController;
  CommandExecutor *commandExecutor;
  const UiHandles *uiHandles;
};

#endif

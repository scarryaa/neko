#ifndef TAB_FLOWS_H
#define TAB_FLOWS_H

#include "features/main_window/interfaces/save_result.h"
#include "features/main_window/ui_handles.h"
#include "features/tabs/types/types.h"
#include "types/ffi_types_fwd.h"
#include <QList>
#include <functional>
#include <string>

class TabController;
class AppStateController;
class EditorController;

/// \class TabFlows
/// \brief Orchestrates tab-related workflows that involve multiple controllers
/// and UI pieces (dialogs, status bar, tab bar, etc.). It is internal to
/// WorkspaceCoordinator.
class TabFlows {
public:
  struct TabFlowsProps {
    TabController *tabController;
    AppStateController *appStateController;
    EditorController *editorController;
    const UiHandles uiHandles;
  };

  explicit TabFlows(const TabFlowsProps &props);
  ~TabFlows() = default;

  // High-level tab commands
  void handleTabCommand(const std::string &commandId,
                        const neko::TabContextFfi &ctx, bool forceClose);

  void closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool forceClose);

  void newTab();
  void tabChanged(int tabId);
  void tabUnpinned(int tabId);
  void moveTabBy(int delta, bool useHistory);

  // Actions on single tabs
  void copyTabPath(int tabId);
  bool revealTab(const neko::TabContextFfi &ctx);
  void tabTogglePin(int tabId, bool isPinned);
  void fileSaved(bool saveAs);

  // Editor / buffer changes
  void bufferChanged();

  // Aggregate tab events
  void handleTabsClosed();
  int getModifiedTabCount(const QList<int> &ids);

  // Save flows
  [[nodiscard]] SaveResult saveTab(int tabId, bool isSaveAs);

  // Scroll offset flows
  void saveScrollOffsetsForActiveTab();
  void restoreScrollOffsetsForActiveTab();
  void restoreScrollOffsetsForReopenedTab(
      const TabScrollOffsets &scrollOffsets) const;

private:
  TabController *tabController;
  AppStateController *appStateController;
  EditorController *editorController;
  const UiHandles uiHandles;

  bool closeManyTabs(const QList<int> &ids, bool forceClose,
                     const std::function<void()> &closeAction);

  [[nodiscard]] bool saveTabWithPromptIfNeeded(int tabId, bool isSaveAs);
};

#endif // TAB_FLOWS_H

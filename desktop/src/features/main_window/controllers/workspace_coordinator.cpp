#include "workspace_coordinator.h"
#include "core/bridge/app_bridge.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/bridge/editor_bridge.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/command_executor.h"
#include "features/main_window/services/app_config_service.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/bridge/tab_bridge.h"
#include "features/tabs/tab_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>

WorkspaceCoordinator::WorkspaceCoordinator(
    const WorkspaceCoordinatorProps &props, QObject *parent)
    : tabBridge(props.tabBridge), appBridge(props.appBridge),
      appConfigService(props.appConfigService),
      editorBridge(props.editorBridge), uiHandles(props.uiHandles),
      commandExecutor(props.commandExecutor),
      tabFlows({.tabBridge = props.tabBridge,
                .appBridge = props.appBridge,
                .editorBridge = props.editorBridge,
                .uiHandles = props.uiHandles}),
      fileExplorerFlows({.appBridge = props.appBridge,
                         .fileTreeBridge = props.fileTreeBridge,
                         .uiHandles = props.uiHandles}),
      QObject(parent) {
  // TabBridge -> WorkspaceCoordinator
  connect(tabBridge, &TabBridge::activeTabChanged, this,
          &WorkspaceCoordinator::refreshUiForActiveTab);
  connect(tabBridge, &TabBridge::allTabsClosed, this,
          [this] { refreshUiForActiveTab(false); });
  connect(tabBridge, &TabBridge::restoreScrollOffsetsForReopenedTab, this,
          [this](const TabScrollOffsets &scrollOffsets) {
            tabFlows.restoreScrollOffsetsForReopenedTab(scrollOffsets);
          });

  // TabBridge -> TabBarWidget
  connect(tabBridge, &TabBridge::tabOpened, uiHandles.tabBarWidget,
          &TabBarWidget::addTab);
  connect(tabBridge, &TabBridge::tabClosed, uiHandles.tabBarWidget,
          &TabBarWidget::removeTab);
  connect(tabBridge, &TabBridge::tabMoved, uiHandles.tabBarWidget,
          [this](int fromIndex, int toIndex) {
            uiHandles.tabBarWidget->moveTab(fromIndex, toIndex);
            uiHandles.editorWidget->setFocus();
          });
  connect(tabBridge, &TabBridge::tabUpdated, uiHandles.tabBarWidget,
          &TabBarWidget::updateTab);
  connect(tabBridge, &TabBridge::activeTabChanged, uiHandles.tabBarWidget,
          &TabBarWidget::setCurrentTabId);

  // WorkspaceCoordinator -> TabBridge
  connect(this, &WorkspaceCoordinator::fileOpened, tabBridge,
          &TabBridge::fileOpened);

  // WorkspaceCoordinator -> FileExplorerWidget
  connect(this, &WorkspaceCoordinator::requestFileExplorerRedraw,
          uiHandles.fileExplorerWidget, &FileExplorerWidget::redraw);
  connect(this, &WorkspaceCoordinator::requestFileExplorerSizeUpdate,
          uiHandles.fileExplorerWidget, &FileExplorerWidget::updateDimensions);

  // FileExplorerWidget -> FileExplorerFlows
  connect(uiHandles.fileExplorerWidget, &FileExplorerWidget::commandRequested,
          [this](auto commandId, auto ctx, auto bypassDeleteConfirmation) {
            fileExplorerFlows.handleFileExplorerCommand(
                commandId, ctx, bypassDeleteConfirmation);
          });

  auto editorController = appBridge->getEditorController();
  setEditorController(std::move(editorController));
}

void WorkspaceCoordinator::fileExplorerToggled() {
  const bool shouldShow = uiHandles.fileExplorerWidget->isHidden();
  uiHandles.fileExplorerWidget->setVisible(shouldShow);

  auto snapshot = appConfigService->getSnapshot();
  snapshot.file_explorer.shown = shouldShow;
  appConfigService->setFileExplorerShown(shouldShow);

  emit onFileExplorerToggledViaShortcut(
      !uiHandles.fileExplorerWidget->isHidden());
}

void WorkspaceCoordinator::cursorPositionClicked() {
  const auto snapshot = tabBridge->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  const auto cursor = editorBridge->getLastAddedCursor();
  const int lineCount = editorBridge->getLineCount();

  if (lineCount == 0) {
    return;
  }

  const auto maxCol =
      std::max<std::size_t>(1, editorBridge->getLineLength(cursor.row));
  const auto lastLineMaxCol =
      std::max<std::size_t>(1, editorBridge->getLineLength(lineCount - 1));

  uiHandles.commandPaletteWidget->showPalette(
      CommandPaletteMode::Jump,
      {
          .maxLineCount = lineCount,
          .maxColumn = static_cast<int>(maxCol),
          .lastLineMaxColumn = static_cast<int>(lastLineMaxCol),
          .maxRow = lineCount,
          .currentRow = cursor.row,
          .currentColumn = cursor.column,
      });
}

std::vector<ShortcutHintRow>
WorkspaceCoordinator::buildJumpHintRows(AppBridge *appBridge) {
  std::vector<ShortcutHintRow> result;
  const auto jumpCommands = appBridge->getAvailableJumpCommands();

  result.reserve(jumpCommands.size());
  for (const auto &cmd : jumpCommands) {
    result.push_back({
        QString::fromUtf8(cmd.key),
        QString::fromUtf8(cmd.display_name),
    });
  }

  return result;
}

std::optional<QString>
WorkspaceCoordinator::requestFileExplorerDirectory() const {
  QString dir = DialogService::openDirectorySelectionDialog(uiHandles.window);

  if (dir.isEmpty()) {
    return std::nullopt;
  }

  return dir;
}

void WorkspaceCoordinator::commandPaletteGoToPosition(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const QString &jumpCommandKey, int64_t row, int64_t column,
    bool isPosition) {
  const auto snapshot = tabBridge->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  if (isPosition) {
    const int lineCount = editorBridge->getLineCount();
    const int64_t maxLine = std::max<int64_t>(1, lineCount);

    const int64_t clampedRow = std::clamp<int64_t>(row, 1, maxLine);
    const int64_t clampedCol = std::max<int64_t>(1, column);
    const auto adjustedRow = static_cast<std::int64_t>(clampedRow - 1);
    const auto adjustedColumn = static_cast<std::int64_t>(clampedCol - 1);

    neko::JumpCommandFfi jumpCommand{
        .kind = neko::JumpCommandKindFfi::ToPosition,
        .row = adjustedRow,
        .column = adjustedColumn,
        .line_target = neko::LineTargetFfi::Start,
        .document_target = neko::DocumentTargetFfi::Start,
    };

    appBridge->executeJumpCommand(jumpCommand);
  } else {
    appBridge->executeJumpKey(jumpCommandKey);
  }

  // TODO(scarlet): Turn these into signals?
  uiHandles.editorWidget->setFocus();
  uiHandles.editorWidget->onCursorChanged();
  uiHandles.gutterWidget->onCursorChanged();
  refreshStatusBarCursorInfo();
}

void WorkspaceCoordinator::commandPaletteCommand(const QString &key,
                                                 const QString &fullText) {
  rust::String rustKey;
  rust::String displayName;
  neko::CommandKindFfi kind;
  rust::String argument;

  const auto commands = appBridge->getAvailableCommands();
  for (const auto &nekoCommand : commands) {
    if (key == QString::fromUtf8(nekoCommand.key)) {
      rustKey = nekoCommand.key;
      displayName = nekoCommand.display_name;
      kind = nekoCommand.kind;

      if (kind == neko::CommandKindFfi::JumpManagement) {
        argument = rust::String(fullText.toStdString());
      } else {
        argument = nekoCommand.argument;
      }

      break;
    }
  }

  if (rustKey.size() == 0) {
    qDebug() << "commandPaletteCommand: unknown key" << key
             << "fullText:" << fullText;
    return;
  }

  const auto beforeSnapshot = tabBridge->getTabsSnapshot();
  if (beforeSnapshot.active_present) {
    tabFlows.saveScrollOffsetsForActiveTab();
  }

  auto result = commandExecutor->execute(rustKey, displayName, kind, argument);

  for (auto &intent : result.intents) {
    switch (intent.kind) {
    case neko::UiIntentKindFfi::ToggleFileExplorer:
      fileExplorerToggled();
      break;
    case neko::UiIntentKindFfi::ApplyTheme:
      emit themeChanged(std::string(intent.argument_str.c_str()));
      break;
    case neko::UiIntentKindFfi::OpenConfig: {
      const int tabId = static_cast<int>(intent.argument_u64);
      const std::string &path = std::string(intent.argument_str);

      // If config was already open, just activate it
      for (const auto &tab : beforeSnapshot.tabs) {
        if (tab.path_present && tab.path == path) {
          tabBridge->setActiveTab(static_cast<int>(tab.id));
          return;
        }
      }

      tabBridge->notifyTabOpenedFromCore(tabId);
      break;
    }
    case neko::UiIntentKindFfi::ShowJumpAliases: {
      // const auto aliases = intent.jump_aliases;
      // TODO(scarlet): Show jump aliases
      break;
    }
    }
  }
}

QString WorkspaceCoordinator::getInitialDialogDirectory() const {
  const auto snapshot = tabBridge->getTabsSnapshot();

  if (!snapshot.active_present) {
    return QDir::homePath();
  }

  // Find the active tab
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == snapshot.active_id && tab.path_present) {
      const QString path = QString::fromUtf8(tab.path.data());
      QFileInfo fileInfo(path);

      return fileInfo.isDir() ? fileInfo.absoluteFilePath()
                              : fileInfo.absolutePath();
    }
  }

  return QDir::homePath();
}

void WorkspaceCoordinator::performFileOpen(const QString &path) {
  // Save scroll offsets for the current tab
  if (const auto snapshot = tabBridge->getTabsSnapshot();
      snapshot.active_present) {
    tabFlows.saveScrollOffsetsForActiveTab();
  }

  const auto openResult = appBridge->openFile(path, true);
  if (openResult.found_tab_id) {
    const int newTabId = static_cast<int>(openResult.tab_id);
    if (openResult.tab_already_exists) {
      // If the tab already existed, just activate it.
      tabBridge->setActiveTab(newTabId);
      return;
    }

    // Otherwise, a new tab was opened.
    const auto newTabSnapshotMaybe = tabBridge->getTabSnapshot(newTabId);

    if (newTabSnapshotMaybe.found) {
      tabBridge->fileOpened(newTabSnapshotMaybe.snapshot);
    }
  }
}

void WorkspaceCoordinator::openFile() {
  const QString initialDir = getInitialDialogDirectory();
  const QString filePath =
      DialogService::openFileSelectionDialog(initialDir, uiHandles.window);

  if (filePath.isEmpty()) {
    return;
  }

  performFileOpen(filePath);
}

void WorkspaceCoordinator::fileSelected(const QString &path, bool focusEditor) {
  performFileOpen(path);

  if (focusEditor) {
    uiHandles.editorWidget->setFocus();
  }
}

void WorkspaceCoordinator::fileSaved(bool saveAs) {
  tabFlows.fileSaved(saveAs);
}

void WorkspaceCoordinator::openConfig() {
  const auto configPath = appConfigService->getConfigPath();

  if (!configPath.isEmpty()) {
    fileSelected(configPath, true);
  }
}

void WorkspaceCoordinator::copyTabPath(int tabId) {
  tabFlows.copyTabPath(tabId);
}

void WorkspaceCoordinator::tabTogglePin(int tabId, bool tabIsPinned) {
  tabFlows.tabTogglePin(tabId, tabIsPinned);
}

void WorkspaceCoordinator::newTab() { tabFlows.newTab(); }

void WorkspaceCoordinator::tabUnpinned(int tabId) {
  tabFlows.tabUnpinned(tabId);
}

void WorkspaceCoordinator::bufferChanged() { tabFlows.bufferChanged(); }

void WorkspaceCoordinator::tabChanged(int tabId) { tabFlows.tabChanged(tabId); }

SaveResult WorkspaceCoordinator::saveTab(int tabId, bool isSaveAs) {
  return tabFlows.saveTab(tabId, isSaveAs);
}

// TODO(scarlet): Merge this with the other tab command handling?
void WorkspaceCoordinator::revealActiveTab() {
  const auto snapshot = tabBridge->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  neko::TabContextFfi ctx;
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == snapshot.active_id) {
      ctx.id = tab.id;
      ctx.is_pinned = tab.pinned;
      ctx.is_modified = tab.modified;
      ctx.file_path_present = tab.path_present;
      ctx.file_path = tab.path;
      break;
    }
  }

  handleCommand("tab.reveal", ctx, false);
}

void WorkspaceCoordinator::moveTabBy(int delta, bool useHistory) {
  tabFlows.moveTabBy(delta, useHistory);
}

// TODO(scarlet): Wrap the neko:: type eventually so it doesn't leak into
// widgets
void WorkspaceCoordinator::closeTabs(
    neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
    bool forceClose) {
  tabFlows.closeTabs(operationType, anchorTabId, forceClose);
}

void WorkspaceCoordinator::applyInitialState() {
  const auto snapshot = tabBridge->getTabsSnapshot();

  int index = 0;
  for (const auto &tab : snapshot.tabs) {
    TabPresentation presentation;
    presentation.id = static_cast<int>(tab.id);
    presentation.title = QString::fromUtf8(tab.title);
    presentation.path = QString::fromUtf8(tab.path);
    presentation.pinned = tab.pinned;
    presentation.modified = tab.modified;

    uiHandles.tabBarWidget->addTab(presentation, index++);
  }

  if (snapshot.active_present) {
    uiHandles.tabBarWidget->setCurrentTabId(
        static_cast<int>(snapshot.active_id));
  }

  refreshStatusBarCursorInfo();

  auto cfg = appConfigService->getSnapshot();
  if (!cfg.file_explorer.shown) {
    uiHandles.fileExplorerWidget->hide();
  }

  uiHandles.editorWidget->setFocus();
}

void WorkspaceCoordinator::refreshUiForActiveTab(bool focusEditor) {
  const auto snapshot = tabBridge->getTabsSnapshot();

  if (!snapshot.active_present) {
    uiHandles.tabBarContainerWidget->hide();
    uiHandles.editorWidget->hide();
    uiHandles.gutterWidget->hide();
    uiHandles.emptyStateWidget->show();
    uiHandles.fileExplorerWidget->setFocus();
    return;
  }

  // IMPORTANT: Set active editor before re-showing widgets, otherwise
  // Qt layout triggers a resize event (which queries the line count with the
  // old editor reference)
  auto editorController = appBridge->getEditorController();
  setEditorController(std::move(editorController));

  uiHandles.emptyStateWidget->hide();
  uiHandles.tabBarContainerWidget->show();
  uiHandles.editorWidget->show();
  uiHandles.gutterWidget->show();
  uiHandles.statusBarWidget->showCursorPositionInfo();

  uiHandles.editorWidget->updateDimensions();
  uiHandles.editorWidget->redraw();
  uiHandles.gutterWidget->updateDimensions();
  uiHandles.gutterWidget->redraw();

  tabFlows.restoreScrollOffsetsForActiveTab();
  refreshStatusBarCursorInfo();

  if (focusEditor) {
    uiHandles.editorWidget->setFocus();
  }
}

void WorkspaceCoordinator::setEditorController(
    rust::Box<neko::EditorController> editorController) {
  editorBridge->setController(std::move(editorController));

  uiHandles.editorWidget->setEditorBridge(editorBridge);
  uiHandles.gutterWidget->setEditorBridge(editorBridge);
}

void WorkspaceCoordinator::refreshStatusBarCursorInfo() {
  if (editorBridge == nullptr) {
    return;
  }

  const auto cursorPosition = editorBridge->getLastAddedCursor();
  const int numberOfCursors =
      static_cast<int>(editorBridge->getCursorPositions().size());

  uiHandles.statusBarWidget->updateCursorPosition(
      cursorPosition.row, cursorPosition.column, numberOfCursors);
}

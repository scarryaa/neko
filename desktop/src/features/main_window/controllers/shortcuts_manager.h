#ifndef SHORTCUTS_MANAGER_H
#define SHORTCUTS_MANAGER_H

#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/tabs/controllers/tab_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QAction>

class ShortcutsManager : public QObject {
  Q_OBJECT

public:
  explicit ShortcutsManager(QWidget *actionOwner,
                            neko::ShortcutsManager *nekoShortcutsManager,
                            WorkspaceCoordinator *workspaceCoordinator,
                            TabController *tabController,
                            const WorkspaceUiHandles *uiHandles,
                            QObject *parent = nullptr);
  ~ShortcutsManager() override = default;

  void setUpKeyboardShortcuts();

private:
  template <typename Slot>
  void addShortcut(QAction *action, const QKeySequence &sequence,
                   Qt::ShortcutContext context, Slot &&slot);

  QWidget *actionOwner;
  neko::ShortcutsManager *nekoShortcutsManager;
  WorkspaceCoordinator *workspaceCoordinator;
  TabController *tabController;
  const WorkspaceUiHandles *uiHandles;
};

#endif

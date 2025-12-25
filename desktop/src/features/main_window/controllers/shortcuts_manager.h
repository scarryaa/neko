#ifndef SHORTCUTS_MANAGER_H
#define SHORTCUTS_MANAGER_H

class TabController;
class WorkspaceCoordinator;
class WorkspaceUiHandles;

#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QWidget>

QT_FWD(QAction)

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

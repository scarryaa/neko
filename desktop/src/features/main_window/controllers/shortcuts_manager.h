#ifndef SHORTCUTS_MANAGER_H
#define SHORTCUTS_MANAGER_H

class TabController;
class WorkspaceCoordinator;
class UiHandles;

#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QWidget>

QT_FWD(QAction)

class ShortcutsManager : public QObject {
  Q_OBJECT

public:
  struct ShortcutsManagerProps {
    QWidget *actionOwner;
    neko::ShortcutsManager *shortcutsManager;
    WorkspaceCoordinator *workspaceCoordinator;
    TabController *tabController;
    const UiHandles *uiHandles;
  };

  explicit ShortcutsManager(const ShortcutsManagerProps &props,
                            QObject *parent = nullptr);
  ~ShortcutsManager() override = default;

  void setUpKeyboardShortcuts();

private:
  // Populates the shortcutMap and adds any missing shortcuts
  void syncShortcutsFromRust();
  // Creates QActions from the shortcutMap and connects them
  void registerAllShortcuts();

  [[nodiscard]] QKeySequence seqFor(const std::string &key,
                                    const QKeySequence &fallback) const;

  template <typename Slot>
  void registerShortcut(const std::string &key, const QKeySequence &fallback,
                        Qt::ShortcutContext context, Slot &&slot);
  template <typename Slot>
  void addShortcut(QAction *action, const QKeySequence &sequence,
                   Qt::ShortcutContext context, Slot &&slot);

  std::unordered_map<std::string, std::string> shortcutMap;
  QWidget *actionOwner;
  neko::ShortcutsManager *nekoShortcutsManager;
  WorkspaceCoordinator *workspaceCoordinator;
  TabController *tabController;
  const UiHandles *uiHandles;
};

#endif

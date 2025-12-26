#ifndef SHORTCUTS_MANAGER_H
#define SHORTCUTS_MANAGER_H

class TabController;
class WorkspaceCoordinator;
class UiHandles;

#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QWidget>
#include <unordered_map>

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
  void populateShortcutMetadata();

  [[nodiscard]] QKeySequence seqFor(const std::string &key,
                                    const QKeySequence &fallback) const;

  // Shortcut scoped to *actionOwner (MainWindow)
  template <typename Slot>
  void registerShortcut(const std::string &key, const QKeySequence &fallback,
                        Qt::ShortcutContext context, Slot &&slot);
  // Shortcut scoped to *owner
  template <typename Slot>
  void registerShortcut(QWidget *owner, const std::string &key,
                        const QKeySequence &fallback,
                        Qt::ShortcutContext context, Slot &&slot);
  // Shortcut scoped to *actionOwner (MainWindow)
  template <typename Slot>
  void addShortcut(QAction *action, const QKeySequence &sequence,
                   Qt::ShortcutContext context, Slot &&slot);
  // Shortcut scoped to *owner
  template <typename Slot>
  void addShortcut(QWidget *owner, QAction *action,
                   const QKeySequence &sequence, Qt::ShortcutContext context,
                   Slot &&slot);

  using ShortcutFn = std::function<void()>;
  struct Shortcut {
    QWidget *owner;
    std::string key;
    std::string keyCombo;
    Qt::ShortcutContext context;
    ShortcutFn action;
  };

  std::unordered_map<std::string, Shortcut> shortcutMap;
  std::unordered_map<std::string, ShortcutFn> shortcutActionMap;

  QWidget *actionOwner;
  neko::ShortcutsManager *nekoShortcutsManager;
  WorkspaceCoordinator *workspaceCoordinator;
  TabController *tabController;
  const UiHandles *uiHandles;
};

#endif

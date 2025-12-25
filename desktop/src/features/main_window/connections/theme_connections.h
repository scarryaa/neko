#ifndef THEME_CONNECTIONS_H
#define THEME_CONNECTIONS_H

class ThemeProvider;

#include "features/main_window/workspace_ui_handles.h"
#include "theme/theme_types.h"
#include <QObject>

class ThemeConnections : public QObject {
  Q_OBJECT

public:
  struct ThemeConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
    ThemeProvider *themeProvider;
  };

  explicit ThemeConnections(const ThemeConnectionsProps &props,
                            QObject *parent = nullptr);
  ~ThemeConnections() override = default;

private:
  void applyNewTabButtonTheme(const NewTabButtonTheme &theme) const;
  void applySplitterTheme(const SplitterTheme &theme) const;
  void applyEmptyStateTheme(const EmptyStateTheme &theme) const;

  WorkspaceUiHandles uiHandles;
};

#endif

#ifndef THEME_CONNECTIONS_H
#define THEME_CONNECTIONS_H

class ThemeProvider;

#include "features/main_window/ui_handles.h"
#include "theme/types/types.h"
#include <QObject>

class ThemeConnections : public QObject {
  Q_OBJECT

public:
  struct ThemeConnectionsProps {
    const UiHandles &uiHandles;
    ThemeProvider *themeProvider;
  };

  explicit ThemeConnections(const ThemeConnectionsProps &props,
                            QObject *parent = nullptr);
  ~ThemeConnections() override = default;

private:
  void applyNewTabButtonTheme(const NewTabButtonTheme &theme) const;
  void applySplitterTheme(const SplitterTheme &theme) const;
  void applyEmptyStateTheme(const EmptyStateTheme &theme) const;

  UiHandles uiHandles;
};

#endif

#ifndef THEME_PROVIDER_H
#define THEME_PROVIDER_H

#include "theme/theme_types.h"
#include "types/ffi_types_fwd.h"
#include <QObject>

class ThemeProvider : public QObject {
  Q_OBJECT

public:
  explicit ThemeProvider(neko::ThemeManager *themeManager,
                         QObject *parent = nullptr);

  [[nodiscard]] const TitleBarTheme &getTitleBarTheme() const;
  [[nodiscard]] const FileExplorerTheme &getFileExplorerTheme() const;
  [[nodiscard]] const CommandPaletteTheme &getCommandPaletteTheme() const;
  [[nodiscard]] const TabBarTheme &getTabBarTheme() const;
  [[nodiscard]] const TabTheme &getTabTheme() const;
  [[nodiscard]] const EditorTheme &getEditorTheme() const;
  [[nodiscard]] const GutterTheme &getGutterTheme() const;
  [[nodiscard]] const StatusBarTheme &getStatusBarTheme() const;
  [[nodiscard]] const ScrollBarTheme &getScrollBarTheme() const;
  [[nodiscard]] const NewTabButtonTheme &getNewTabButtonTheme() const;
  [[nodiscard]] const SplitterTheme &getSplitterTheme() const;
  [[nodiscard]] const EmptyStateTheme &getEmptyStateTheme() const;

signals:
  void titleBarThemeChanged(const TitleBarTheme &titleBarTheme);
  void fileExplorerThemeChanged(const FileExplorerTheme &fileExplorerTheme);
  void editorThemeChanged(const EditorTheme &editorTheme);
  void gutterThemeChanged(const GutterTheme &gutterTheme);
  void statusBarThemeChanged(const StatusBarTheme &statusBarTheme);
  void tabBarThemeChanged(const TabBarTheme &tabBarTheme);
  void tabThemeChanged(const TabTheme &tabTheme);
  void
  commandPaletteThemeChanged(const CommandPaletteTheme &commandPaletteTheme);
  void scrollBarThemeChanged(const ScrollBarTheme &scrollBarTheme);
  void newTabButtonThemeChanged(const NewTabButtonTheme &theme);
  void splitterThemeChanged(const SplitterTheme &theme);
  void emptyStateThemeChanged(const EmptyStateTheme &theme);

public slots:
  void reload();

private:
  void refreshTitleBarTheme();
  void refreshFileExplorerTheme();
  void refreshCommandPaletteTheme();
  void refreshTabBarTheme();
  void refreshTabTheme();
  void refreshEditorTheme();
  void refreshGutterTheme();
  void refreshStatusBarTheme();
  void refreshScrollBarTheme();
  void refreshNewTabButtonTheme();
  void refreshSplitterTheme();
  void refreshEmptyStateTheme();

  neko::ThemeManager *themeManager;

  TitleBarTheme titleBarTheme;
  FileExplorerTheme fileExplorerTheme;
  CommandPaletteTheme commandPaletteTheme;
  TabBarTheme tabBarTheme;
  TabTheme tabTheme;
  EditorTheme editorTheme;
  GutterTheme gutterTheme;
  StatusBarTheme statusBarTheme;
  ScrollBarTheme scrollBarTheme;
  NewTabButtonTheme newTabButtonTheme;
  SplitterTheme splitterTheme;
  EmptyStateTheme emptyStateTheme;
};

#endif

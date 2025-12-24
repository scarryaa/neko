#ifndef THEME_TYPES_H
#define THEME_TYPES_H

#include <QString>

struct ScrollBarTheme {
  QString thumbColor;
  QString thumbHoverColor;
};

struct EmptyStateTheme {
  QString backgroundColor;
  QString buttonBackgroundColor;
  QString buttonForegroundColor;
  QString buttonHoverColor;
  QString borderColor;
};

struct NewTabButtonTheme {
  QString backgroundColor;
  QString foregroundColor;
  QString hoverColor;
  QString borderColor;
};

struct SplitterTheme {
  QString handleColor;
  int handleWidth = 1;
};

struct TitleBarTheme {
  QString buttonForegroundColor;
  QString buttonHoverColor;
  QString buttonPressColor;
  QString backgroundColor;
  QString borderColor;
};

struct StatusBarTheme {
  QString backgroundColor;
  QString borderColor;
  QString buttonForegroundColor;
  QString buttonHoverColor;
  QString buttonPressColor;
  QString foregroundMutedColor;
  QString accentColor;
};

struct CommandPaletteTheme {
  QString backgroundColor;
  QString borderColor;
  QString foregroundColor;
  QString foregroundVeryMutedColor;
  QString accentMutedColor;
  QString accentForegroundColor;
  QString shadowColor;
};

struct ContextMenuTheme {
  QString backgroundColor;
  QString borderColor;
  QString labelColor;
  QString labelDisabledColor;
  QString shortcutColor;
  QString shortcutDisabledColor;
  QString hoverColor;
  QString accentMutedColor;
  QString accentForegroundColor;
  QString shadowColor;
};

struct FileExplorerTheme {
  QString backgroundColor;
  QString buttonBackgroundColor;
  QString buttonForegroundColor;
  QString buttonHoverColor;
  QString buttonPressColor;
  QString fileForegroundColor;
  QString fileHiddenColor;
  QString selectionColor;
  ScrollBarTheme scrollBarTheme;
};

struct TabBarTheme {
  QString backgroundColor;
  QString indicatorColor;
  QString borderColor;
  ScrollBarTheme scrollBarTheme;
};

struct TabTheme {
  QString tabForegroundColor;
  QString tabForegroundInactiveColor;
  QString tabActiveColor;
  QString tabInactiveColor;
  QString tabHoverColor;
  QString tabModifiedIndicatorColor;
  QString tabCloseButtonHoverColor;
  QString borderColor;
};

struct EditorTheme {
  QString backgroundColor;
  QString foregroundColor;
  QString highlightColor;
  QString accentColor;
  ScrollBarTheme scrollBarTheme;
};

struct GutterTheme {
  QString backgroundColor;
  QString foregroundColor;
  QString foregroundActiveColor;
  QString accentColor;
  QString highlightColor;
};

#endif

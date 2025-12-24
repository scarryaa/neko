#ifndef THEME_PROVIDER_H
#define THEME_PROVIDER_H

#include "theme/theme_types.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class ThemeProvider : public QObject {
  Q_OBJECT

public:
  explicit ThemeProvider(neko::ThemeManager *themeManager,
                         QObject *parent = nullptr);

  [[nodiscard]] const TitleBarTheme &getTitleBarTheme() const;

signals:
  void titleBarThemeChanged(const TitleBarTheme &titleBarTheme);

public slots:
  void reload();

private:
  void refreshTitleBarTheme();

  neko::ThemeManager *themeManager;
  TitleBarTheme titleBarTheme;
};

#endif

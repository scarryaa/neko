#include "gui_utils.h"

QString UiUtils::getConfigString(neko::ConfigManager *manager,
                                 char *(*getter)(neko::ConfigManager *)) {
  auto raw = getter(manager);
  QString result = QString::fromUtf8(raw);

  return result;
}

QString UiUtils::getThemeColor(neko::ThemeManager &manager, const char *key,
                               const char *fallback) {
  auto colorStr = manager.get_color(key);
  QString result = QString::fromUtf8(colorStr);

  if (result.isEmpty()) {
    return fallback;
  }

  return result;
}

QFont UiUtils::loadFont(neko::ConfigManager &manager, neko::FontType type) {
  bool forceMonospace = false;

  auto rawFamily = manager.get_font_family(type);
  auto rawSize = manager.get_font_size(type);

  QString family = QString::fromUtf8(rawFamily);
  QFont font(family, rawSize);

  return font;
}

QString UiUtils::getScrollBarStylesheet(const QString &widgetName,
                                        const QString &bgColor,
                                        const QString &additions) {
  QString stylesheet =
      QString("QAbstractScrollArea::corner { background: transparent; }"
              "QScrollBar:vertical { background: transparent; width: 12px; "
              "margin: 0px; }"
              "QScrollBar::handle:vertical { background: #555555; "
              "min-height: 20px; }"
              "QScrollBar::handle:vertical:hover { background: #666666; }"
              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical "
              "{ height: 0px; }"
              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical "
              "{ background: none; }"
              "QScrollBar:horizontal { background: transparent; height: "
              "12px; margin: 0px; }"
              "QScrollBar::handle:horizontal { background: #555555; "
              "min-width: 20px; }"
              "QScrollBar::handle:horizontal:hover { background: #666666; }"
              "QScrollBar::add-line:horizontal, "
              "QScrollBar::sub-line:horizontal { width: 0px; }"
              "QScrollBar::add-page:horizontal, "
              "QScrollBar::sub-page:horizontal { background: none; }"
              "%1 { background: %2; %3; }")
          .arg(widgetName, bgColor, additions);

  return stylesheet;
}

double UiUtils::getTitleBarContentMargin() {
#if defined(Q_OS_MACOS)
  return 84; // Spacing for traffic lights
#else
  return 10;
#endif
}

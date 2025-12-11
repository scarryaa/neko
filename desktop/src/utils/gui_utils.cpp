#include "gui_utils.h"

QString UiUtils::getConfigString(NekoConfigManager *manager,
                                 char *(*getter)(NekoConfigManager *)) {
  char *raw = getter(manager);
  QString result;

  if (raw) {
    result = QString::fromUtf8(raw);
    neko_string_free(raw);
  }

  return result;
}

QString UiUtils::getThemeColor(NekoThemeManager *manager, const char *key,
                               const char *fallback) {
  char *raw = neko_theme_get_color(manager, key);
  QString result = fallback;

  if (raw) {
    result = QString::fromUtf8(raw);
    neko_string_free(raw);
  }

  return result;
}

QFont UiUtils::loadFont(NekoConfigManager *manager, FontType type) {
  char *rawFamily = nullptr;
  size_t size = 15;
  bool forceMonospace = false;

  switch (type) {
  case FontType::Editor:
    rawFamily = neko_config_get_editor_font_family(manager);
    size = neko_config_get_editor_font_size(manager);
    forceMonospace = true;
    break;
  case FontType::FileExplorer:
    rawFamily = neko_config_get_file_explorer_font_family(manager);
    size = neko_config_get_file_explorer_font_size(manager);
    forceMonospace = false;
    break;
  case FontType::Interface:
    size = 15;
    break;
  case FontType::Terminal:
    size = 15;
    forceMonospace = true;
    break;
  }

  QString family;
  if (rawFamily) {
    family = QString::fromUtf8(rawFamily);
    neko_string_free(rawFamily);
  } else {
    if (forceMonospace) {
      family = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    } else {
      family = QApplication::font().family();
    }
  }

  QFont font(family, size);

  if (forceMonospace) {
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
  } else {
    font.setStyleHint(QFont::SansSerif);
  }

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

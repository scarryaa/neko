#include "gui_utils.h"

QString UiUtils::getConfigString(neko::ConfigManager *manager,
                                 char *(*getter)(neko::ConfigManager *)) {
  auto *raw = getter(manager);
  QString result = QString::fromUtf8(raw);

  return result;
}

// NOLINTNEXTLINE
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

  auto snapshot = manager.get_config_snapshot();
  rust::String rawFamily;
  double size;

  switch (type) {
  case neko::FontType::Interface:
    rawFamily = snapshot.interface_font_family;
    size = snapshot.interface_font_size;
    break;
  case neko::FontType::Editor:
    rawFamily = snapshot.editor_font_family;
    size = snapshot.editor_font_size;
    break;
  case neko::FontType::FileExplorer:
    rawFamily = snapshot.file_explorer_font_family;
    size = snapshot.file_explorer_font_size;
    break;
  case neko::FontType::Terminal:
    rawFamily = snapshot.terminal_font_family;
    size = snapshot.interface_font_size;
    break;
  }

  if (rawFamily.empty() || (size == 0.0)) {
    return {};
  }

  QString family = QString::fromUtf8(rawFamily);
  QFont font(family, static_cast<int>(size));
  return font;
}

void UiUtils::setFontSize(neko::ConfigManager &manager, neko::FontType type,
                          double newFontSize) {
  auto snapshot = manager.get_config_snapshot();

  switch (type) {
  case neko::FontType::Editor:
    snapshot.editor_font_size = static_cast<int>(newFontSize);
    break;
  case neko::FontType::FileExplorer:
    snapshot.file_explorer_font_size = static_cast<int>(newFontSize);
    break;
  case neko::FontType::Interface:
    snapshot.interface_font_size = static_cast<int>(newFontSize);
    break;
  case neko::FontType::Terminal:
    snapshot.terminal_font_size = static_cast<int>(newFontSize);
    break;
  }

  manager.apply_config_snapshot(snapshot);
}

QString UiUtils::getScrollBarStylesheet(neko::ThemeManager &themeManager,
                                        const QString &widgetName,
                                        const QString &bgColor,
                                        const QString &additions) {
  auto scrollbarThumbColor = themeManager.get_color("ui.scrollbar.thumb");
  auto scrollbarThumbHoverColor =
      themeManager.get_color("ui.scrollbar.thumb.hover");

  QString stylesheet =
      QString("QAbstractScrollArea::corner { background: transparent; }"
              "QScrollBar:vertical { background: transparent; width: 12px; "
              "margin: 0px; }"
              "QScrollBar::handle:vertical { background: %1; "
              "min-height: 20px; }"
              "QScrollBar::handle:vertical:hover { background: %2; }"
              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical "
              "{ height: 0px; }"
              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical "
              "{ background: none; }"
              "QScrollBar:horizontal { background: transparent; height: "
              "12px; margin: 0px; }"
              "QScrollBar::handle:horizontal { background: %1; "
              "min-width: 20px; }"
              "QScrollBar::handle:horizontal:hover { background: %2; }"
              "QScrollBar::add-line:horizontal, "
              "QScrollBar::sub-line:horizontal { width: 0px; }"
              "QScrollBar::add-page:horizontal, "
              "QScrollBar::sub-page:horizontal { background: none; }"
              "%3 { background: %4; %5; }")
          .arg(scrollbarThumbColor, scrollbarThumbHoverColor, widgetName,
               bgColor, additions);

  return stylesheet;
}

double UiUtils::getTitleBarContentMargin() {
#if defined(Q_OS_MACOS)
  return MACOS_TRAFFIC_LIGHTS_MARGIN;
#else
  return 10;
#endif
}

QIcon UiUtils::createColorizedIcon(const QIcon &originalIcon,
                                   const QColor &color, const QSize &size) {
  QPixmap pixmap = originalIcon.pixmap(size);

  QPainter painter(&pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

  painter.fillRect(pixmap.rect(), color);
  painter.end();

  return {pixmap};
}

// NOLINTNEXTLINE
QLabel *UiUtils::createLabel(const QString &text, const QString &styleSheet,
                             const QFont &font, QWidget *parent, bool wordWrap,
                             QSizePolicy::Policy sizePolicyHorizontal,
                             QSizePolicy::Policy sizePolicyVertical) {
  auto *label = new QLabel(text, parent);
  label->setStyleSheet(styleSheet);
  label->setWordWrap(wordWrap);
  label->setSizePolicy(sizePolicyHorizontal, sizePolicyVertical);
  label->setFont(font);

  return label;
}

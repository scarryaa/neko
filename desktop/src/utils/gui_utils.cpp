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

void UiUtils::setFontSize(neko::ConfigManager &manager, neko::FontType type,
                          double newFontSize) {
  manager.set_font_size(newFontSize, type);
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
  return 84; // Spacing for traffic lights
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

  return QIcon(pixmap);
}

QLabel *UiUtils::createLabel(QString text, QString styleSheet, QFont font,
                             QWidget *parent, bool wordWrap,
                             QSizePolicy::Policy sizePolicyHorizontal,
                             QSizePolicy::Policy sizePolicyVertical) {
  QLabel *label = new QLabel(text, parent);
  label->setStyleSheet(styleSheet);
  label->setWordWrap(wordWrap);
  label->setSizePolicy(sizePolicyHorizontal, sizePolicyVertical);
  label->setFont(font);

  return label;
}

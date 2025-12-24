#ifndef UI_UTILS_H
#define UI_UTILS_H

#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QSize>
#include <QString>

namespace UiUtils {
QString getConfigString(neko::ConfigManager *manager,
                        char *(getter)(neko::ConfigManager *));

QString getThemeColor(neko::ThemeManager &manager, const char *key,
                      const char *fallback = "#000000");

template <typename... Args>
auto getThemeColors(neko::ThemeManager &manager, Args &&...keys) {
  return std::make_tuple(
      QString::fromUtf8(manager.get_color(std::forward<Args>(keys)))...);
}

QFont loadFont(neko::ConfigManager &manager, neko::FontType type);

void setFontSize(neko::ConfigManager &manager, neko::FontType type,
                 double newFontSize);

QString getScrollBarStylesheet(neko::ThemeManager &themeManager,
                               const QString &widgetName,
                               const QString &bgColor,
                               const QString &additions = nullptr);

QIcon createColorizedIcon(const QIcon &originalIcon, const QColor &color,
                          const QSize &size);

QLabel *
createLabel(const QString &text, const QString &styleSheet,
            const QFont &font = QFont(), QWidget *parent = nullptr,
            bool wordWrap = true,
            QSizePolicy::Policy sizePolicyHorizontal = QSizePolicy::Expanding,
            QSizePolicy::Policy sizePolicyVertical = QSizePolicy::Expanding);
} // namespace UiUtils

#endif // UI_UTILS_H

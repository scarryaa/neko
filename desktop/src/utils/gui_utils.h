#ifndef UI_UTILS_H
#define UI_UTILS_H

#include "neko-core/src/ffi/mod.rs.h"
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QPainter>
#include <QSize>
#include <QString>

namespace UiUtils {
QString getConfigString(neko::ConfigManager *manager,
                        char *(getter)(neko::ConfigManager *));
QString getThemeColor(neko::ThemeManager &manager, const char *key,
                      const char *fallback = "#000000");
QFont loadFont(neko::ConfigManager &manager, neko::FontType type);
void setFontSize(neko::ConfigManager &manager, neko::FontType type,
                 double newFontSize);
QString getScrollBarStylesheet(const QString &widgetName,
                               const QString &bgColor,
                               const QString &additions = nullptr);
double getTitleBarContentMargin();
QIcon createColorizedIcon(const QIcon &originalIcon, const QColor &color,
                          const QSize &size);
} // namespace UiUtils

#endif // UI_UTILS_H

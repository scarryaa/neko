#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QString>
#include <neko_core.h>

namespace UiUtils {
enum class FontType { Editor, FileExplorer, Interface, Terminal };

QString getConfigString(NekoConfigManager *manager,
                        char *(getter)(NekoConfigManager *));
QString getThemeColor(NekoThemeManager *manager, const char *key,
                      const char *fallback = "#000000");
QFont loadFont(NekoConfigManager *manager, FontType type);
QString getScrollBarStylesheet(const QString &widgetName,
                               const QString &bgColor,
                               const QString &additions = nullptr);
double getTitleBarContentMargin();
} // namespace UiUtils

#endif // UI_UTILS_H

#include "main_window.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <neko_core.h>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  auto *buffer = neko_buffer_new();
  neko_buffer_insert(buffer, 0, "Hello from Rust!", 16);

  size_t len;
  const char *text = neko_buffer_get_text(buffer, &len);
  qDebug() << "Buffer content:" << QString::fromUtf8(text, len);
  neko_string_free((char *)text);
  neko_buffer_free(buffer);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "neko_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  MainWindow w;
  w.show();
  return a.exec();
}

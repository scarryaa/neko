#ifndef CONFIG_H
#define CONFIG_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

class Config {
public:
  Config(double editorFontSize, double fileExplorerFontSize);
  Config();

  void parse(QByteArray qb);
  QByteArray toJson();

  double editorFontSize;
  double fileExplorerFontSize;
  QString fileExplorerDirectory;
};

#endif // CONFIG_H

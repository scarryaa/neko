#ifndef CONFIG_H
#define CONFIG_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

class Config {
public:
  Config(double editorFontSize, double fileExplorerFontSize);
  Config();

  void parse(QByteArray qb);
  QByteArray toJson();

  double editorFontSize;
  double fileExplorerFontSize;
};

#endif // CONFIG_H

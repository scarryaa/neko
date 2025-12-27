#ifndef TAB_TYPES_H
#define TAB_TYPES_H

#include <QString>

struct TabPresentation {
  int id;
  QString title;
  QString path;
  bool pinned;
  bool modified;
};

#endif

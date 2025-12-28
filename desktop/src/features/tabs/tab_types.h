#ifndef TAB_TYPES_H
#define TAB_TYPES_H

#include <QString>

struct TabScrollOffsets {
  double x;
  double y;
};

struct TabPresentation {
  int id;
  QString title;
  QString path;
  bool pinned;
  bool modified;
  TabScrollOffsets scrollOffsets;
};

#endif

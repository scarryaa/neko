#ifndef EDITOR_TYPES_H
#define EDITOR_TYPES_H

struct Cursor {
  int row;
  int column;
};

struct Selection {
  Cursor start;
  Cursor end;
  Cursor anchor;
  bool active;
};

#endif

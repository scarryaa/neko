#ifndef CONTEXT_MENU_FRAME_H
#define CONTEXT_MENU_FRAME_H

#include "utils/gui_utils.h"
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <neko-core/src/ffi/bridge.rs.h>

class ContextMenuFrame : public QFrame {
  Q_OBJECT

public:
  explicit ContextMenuFrame(neko::ThemeManager &themeManager,
                            QWidget *parent = nullptr);
  ~ContextMenuFrame();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  neko::ThemeManager &themeManager;
};

#endif // CONTEXT_MENU_FRAME_H

#ifndef PALETTE_FRAME_H
#define PALETTE_FRAME_H

#include "utils/gui_utils.h"
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <neko-core/src/ffi/bridge.rs.h>

class PaletteFrame : public QFrame {
  Q_OBJECT

public:
  explicit PaletteFrame(neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~PaletteFrame() override = default;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  neko::ThemeManager &themeManager;
};

#endif // PALETTE_FRAME_H

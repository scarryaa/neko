#ifndef PALETTE_FRAME_H
#define PALETTE_FRAME_H

#include "types/qt_types_fwd.h"
#include <QFrame>
#include <QString>

QT_FWD(QPaintEvent)

class PaletteFrame : public QFrame {
  Q_OBJECT

private:
  struct PaletteFrameTheme {
    QString backgroundColor;
    QString borderColor;
  };

  struct PaletteFrameProps {
    const PaletteFrameTheme &theme;
  };

public:
  explicit PaletteFrame(const PaletteFrameProps &props,
                        QWidget *parent = nullptr);
  ~PaletteFrame() override = default;

  void setAndApplyTheme(const PaletteFrameTheme &newTheme);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  PaletteFrameTheme theme;
};

#endif // PALETTE_FRAME_H

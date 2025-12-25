#ifndef PALETTE_DIVIDER_H
#define PALETTE_DIVIDER_H

#include "types/qt_types_fwd.h"
#include <QColor>
#include <QWidget>

QT_FWD(QPaintEvent)

class PaletteDivider : public QWidget {
  Q_OBJECT

public:
  struct PaletteDividerProps {
    const QColor &color;
  };

  explicit PaletteDivider(const PaletteDividerProps &props,
                          QWidget *parent = nullptr);
  ~PaletteDivider() override = default;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  const QColor color;

  static constexpr double HEIGHT_DIVIDER = 2.0;
};

#endif // PALETTE_DIVIDER_H

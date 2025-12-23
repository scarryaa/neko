#ifndef PALETTE_DIVIDER_H
#define PALETTE_DIVIDER_H

#include <QPainter>
#include <QWidget>

class PaletteDivider : public QWidget {
  Q_OBJECT

public:
  explicit PaletteDivider(const QColor &color, QWidget *parent = nullptr);
  ~PaletteDivider() override = default;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  const QColor color;

  static constexpr double HEIGHT_DIVIDER = 2.0;
};

#endif // PALETTE_DIVIDER_H

#ifndef PALETTE_DIVIDER_H
#define PALETTE_DIVIDER_H

#include <QPainter>
#include <QWidget>

class PaletteDivider : public QWidget {
  Q_OBJECT

public:
  explicit PaletteDivider(const QColor &color, QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  const QColor color;
};

#endif // PALETTE_DIVIDER_H

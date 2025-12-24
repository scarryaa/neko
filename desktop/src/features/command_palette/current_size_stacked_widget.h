#ifndef CURRENT_SIZE_STACKED_WIDGET_H
#define CURRENT_SIZE_STACKED_WIDGET_H

#include <QStackedWidget>

class CurrentSizeStackedWidget : public QStackedWidget {
  Q_OBJECT

public:
  using QStackedWidget::QStackedWidget;

  [[nodiscard]] QSize sizeHint() const override;
  [[nodiscard]] QSize minimumSizeHint() const override;
};

#endif

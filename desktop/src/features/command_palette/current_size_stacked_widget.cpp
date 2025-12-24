#include "current_size_stacked_widget.h"

QSize CurrentSizeStackedWidget::sizeHint() const {
  if (auto *widget = currentWidget()) {
    return widget->sizeHint();
  }

  return QStackedWidget::sizeHint();
}

QSize CurrentSizeStackedWidget::minimumSizeHint() const {
  if (auto *widget = currentWidget()) {
    return widget->minimumSizeHint();
  }

  return QStackedWidget::minimumSizeHint();
}

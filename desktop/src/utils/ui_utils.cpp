#include "ui_utils.h"
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QWidget>

QFont UiUtils::makeFont(const QString &fontFamily, size_t fontSize) {
  return {fontFamily, static_cast<int>(fontSize)};
}

QString UiUtils::getScrollBarStylesheet(const QString &scrollbarThumbColor,
                                        const QString &scrollbarThumbHoverColor,
                                        const QString &widgetName,
                                        const QString &bgColor,
                                        const QString &additions) {
  QString stylesheet =
      QString("QAbstractScrollArea::corner { background: transparent; }"
              "QScrollBar:vertical { background: transparent; width: 12px; "
              "margin: 0px; }"
              "QScrollBar::handle:vertical { background: %1; "
              "min-height: 20px; }"
              "QScrollBar::handle:vertical:hover { background: %2; }"
              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical "
              "{ height: 0px; }"
              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical "
              "{ background: none; }"
              "QScrollBar:horizontal { background: transparent; height: "
              "12px; margin: 0px; }"
              "QScrollBar::handle:horizontal { background: %1; "
              "min-width: 20px; }"
              "QScrollBar::handle:horizontal:hover { background: %2; }"
              "QScrollBar::add-line:horizontal, "
              "QScrollBar::sub-line:horizontal { width: 0px; }"
              "QScrollBar::add-page:horizontal, "
              "QScrollBar::sub-page:horizontal { background: none; }"
              "%3 { background: %4; %5; }")
          .arg(scrollbarThumbColor, scrollbarThumbHoverColor, widgetName,
               bgColor, additions);

  return stylesheet;
}

QIcon UiUtils::createColorizedIcon(const QIcon &originalIcon,
                                   const QColor &color, const QSize &size) {
  QPixmap pixmap = originalIcon.pixmap(size);

  QPainter painter(&pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

  painter.fillRect(pixmap.rect(), color);
  painter.end();

  return {pixmap};
}

// NOLINTNEXTLINE
QLabel *UiUtils::createLabel(const QString &text, const QString &styleSheet,
                             const QFont &font, QWidget *parent, bool wordWrap,
                             QSizePolicy::Policy sizePolicyHorizontal,
                             QSizePolicy::Policy sizePolicyVertical) {
  auto *label = new QLabel(text, parent);
  label->setStyleSheet(styleSheet);
  label->setWordWrap(wordWrap);
  label->setSizePolicy(sizePolicyHorizontal, sizePolicyVertical);
  label->setFont(font);

  return label;
}

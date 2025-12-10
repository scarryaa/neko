#include "title_bar_widget.h"

TitleBarWidget::TitleBarWidget(QWidget *parent) : QWidget(parent) {
  setFixedHeight(TITLEBAR_HEIGHT);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  m_directorySelectionButton = new QPushButton("Select a directory");
  m_directorySelectionButton->setStyleSheet("QPushButton {"
                                            "  background-color: transparent;"
                                            "  color: #a0a0a0;"
                                            "  border-radius: 6px;"
                                            "  padding: 4px 8px;"
                                            "  font-size: 13px;"
                                            "}"
                                            "QPushButton:hover {"
                                            "  background-color: #131313;"
                                            "}"
                                            "QPushButton:pressed {"
                                            "  background-color: #222222;"
                                            "}");

  connect(m_directorySelectionButton, &QPushButton::clicked, this,
          &TitleBarWidget::onDirectorySelectionButtonPressed);

  QHBoxLayout *layout = new QHBoxLayout(this);

  int leftMargin = 10;
  int rightMargin = 10;

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  leftMargin = 84;
#endif

  layout->setContentsMargins(leftMargin, 5, rightMargin, 5);
  layout->addWidget(m_directorySelectionButton);
  layout->addStretch();
}

TitleBarWidget::~TitleBarWidget() {}

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_clickPos = event->position().toPoint();
  }

  QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    window()->move((event->globalPosition() - m_clickPos).toPoint());

    event->accept();
  }
}

void TitleBarWidget::onDirectorySelectionButtonPressed() {
  emit directorySelectionButtonPressed();
}

void TitleBarWidget::onDirChanged(std::string newDir) {
  QString newDirQStr = QString::fromStdString(newDir);

  m_currentDir = newDirQStr;
  m_directorySelectionButton->setText(newDirQStr.split('/').last());
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setBrush(COLOR_BLACK);
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), TITLEBAR_HEIGHT)));

  painter.setPen(QPen(BORDER_COLOR));
  painter.drawLine(QPointF(0, height() - 1), QPointF(width(), height() - 1));
}

#include "tab_bar_widget.h"

TabBarWidget::TabBarWidget(QWidget *parent) : QScrollArea(parent) {
  setFixedHeight(HEIGHT);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setWidgetResizable(true);
  setAutoFillBackground(false);
  setFrameShape(QFrame::NoFrame);
  viewport()->setStyleSheet(
      "QWidget { background: black; border-bottom: 1px solid #3c3c3c; }");

  containerWidget = new QWidget(this);
  layout = new QHBoxLayout(containerWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  setWidget(containerWidget);

  currentTabIndex = 0;
  viewport()->repaint();
}

TabBarWidget::~TabBarWidget() {}

void TabBarWidget::setTabs(QStringList titles) {
  QLayoutItem *item;
  while ((item = layout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  tabs.clear();

  // Create new tabs
  for (int i = 0; i < titles.size(); i++) {
    auto *tabWidget = new TabWidget(titles[i], i, this);
    connect(tabWidget, &TabWidget::clicked, this, [this, i]() {
      setCurrentIndex(i);
      emit currentChanged(i);
    });
    connect(tabWidget, &TabWidget::closeRequested, this,
            [this, i]() { emit tabCloseRequested(i); });
    layout->addWidget(tabWidget);
    tabs.append(tabWidget);
  }

  layout->addStretch();

  updateTabAppearance();
}

void TabBarWidget::setCurrentIndex(size_t index) {
  if (index < tabs.size()) {
    currentTabIndex = index;
    updateTabAppearance();
  }
}

void TabBarWidget::updateTabAppearance() {
  for (int i = 0; i < tabs.size(); i++) {
    tabs[i]->setActive(i == currentTabIndex);
  }
}

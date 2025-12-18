#include "tab_bar_widget.h"
#include "utils/gui_utils.h"

TabBarWidget::TabBarWidget(neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), configManager(configManager),
      themeManager(themeManager) {
  QFont uiFont = UiUtils::loadFont(configManager, neko::FontType::Interface);
  setFont(uiFont);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;
  setFixedHeight(dynamicHeight);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setWidgetResizable(true);
  setAutoFillBackground(false);
  setFrameShape(QFrame::NoFrame);

  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "tab_bar.background");
  viewport()->setStyleSheet(UiUtils::getScrollBarStylesheet(
      themeManager, "QWidget", backgroundColor,
      QString("border-bottom: 1px solid %1")
          .arg(UiUtils::getThemeColor(themeManager, "ui.border"))));

  containerWidget = new QWidget(this);
  layout = new QHBoxLayout(containerWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setWidget(containerWidget);

  currentTabIndex = 0;
  viewport()->repaint();
}

TabBarWidget::~TabBarWidget() {}

void TabBarWidget::setTabModified(int index, bool modified) {
  tabs[index]->setModified(modified);
}

void TabBarWidget::setTabs(QStringList titles, rust::Vec<bool> modifiedStates) {
  QLayoutItem *item;
  while ((item = layout->takeAt(0)) != nullptr) {
    if (item->widget() && item->widget() != newTabButton) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  tabs.clear();

  // Create new tabs
  int numberOfTitles = titles.size();
  for (int i = 0; i < numberOfTitles; i++) {
    auto *tabWidget =
        new TabWidget(titles[i], i, configManager, themeManager, this);
    tabWidget->setModified(modifiedStates[i]);

    connect(tabWidget, &TabWidget::clicked, this, [this, i]() {
      setCurrentIndex(i);
      emit currentChanged(i);
    });
    connect(tabWidget, &TabWidget::closeRequested, this,
            [this, i, numberOfTitles]() {
              emit tabCloseRequested(i, numberOfTitles);
            });
    layout->addWidget(tabWidget);
    tabs.append(tabWidget);
  }

  layout->addStretch();
  updateTabAppearance();
}

int TabBarWidget::getNumberOfTabs() { return tabs.size(); }

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

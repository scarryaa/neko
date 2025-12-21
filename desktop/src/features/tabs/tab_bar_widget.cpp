#include "tab_bar_widget.h"

TabBarWidget::TabBarWidget(neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager,
                           ContextMenuRegistry &contextMenuRegistry,
                           CommandRegistry &commandRegistry,
                           TabController *tabController, QWidget *parent)
    : QScrollArea(parent), configManager(configManager),
      themeManager(themeManager), contextMenuRegistry(contextMenuRegistry),
      commandRegistry(commandRegistry), tabController(tabController) {
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

  containerWidget = new QWidget(this);
  layout = new QHBoxLayout(containerWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setWidget(containerWidget);

  currentTabIndex = 0;
  registerCommands();
  viewport()->repaint();

  applyTheme();
}

TabBarWidget::~TabBarWidget() {}

void TabBarWidget::applyTheme() {
  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "tab_bar.background");

  viewport()->setStyleSheet(UiUtils::getScrollBarStylesheet(
      themeManager, "QWidget", backgroundColor,
      QString("border-bottom: 1px solid %1")
          .arg(UiUtils::getThemeColor(themeManager, "ui.border"))));

  for (auto *tab : tabs) {
    if (tab) {
      tab->update();
    }
  }
}

void TabBarWidget::setTabModified(int index, bool modified) {
  tabs[index]->setModified(modified);
}

void TabBarWidget::setTabs(QStringList titles, QStringList paths,
                           rust::Vec<bool> modifiedStates) {
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
    auto *tabWidget = new TabWidget(
        titles[i], paths[i], i, configManager, themeManager,
        contextMenuRegistry, commandRegistry,
        [this]() -> int { return tabController->getTabCount(); }, this);
    tabWidget->setModified(modifiedStates[i]);

    connect(tabWidget, &TabWidget::clicked, this, [this, i]() {
      setCurrentIndex(i);
      emit currentChanged(i);
    });
    connect(tabWidget, &TabWidget::closeRequested, this,
            [this, i, numberOfTitles](bool bypassConfirmation) {
              emit tabCloseRequested(i, numberOfTitles, bypassConfirmation);
            });
    layout->addWidget(tabWidget);
    tabs.append(tabWidget);
  }

  layout->addStretch();
  updateTabAppearance();
}

void TabBarWidget::registerCommands() {
  // TODO: Centralize these
  commandRegistry.registerCommand("tab.copyPath", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    QGuiApplication::clipboard()->setText(ctx.filePath);
  });
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

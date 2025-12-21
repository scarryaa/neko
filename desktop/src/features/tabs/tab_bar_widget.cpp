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
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
  viewport()->installEventFilter(this);

  containerWidget = new QWidget(this);
  layout = new QHBoxLayout(containerWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setWidget(containerWidget);

  dropIndicator = new QWidget(containerWidget);
  dropIndicator->setFixedWidth(2);
  dropIndicator->setVisible(false);

  currentTabIndex = 0;
  registerCommands();
  viewport()->update();

  applyTheme();
}

TabBarWidget::~TabBarWidget() {}

void TabBarWidget::applyTheme() {
  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "tab_bar.background");
  auto indicatorColor = UiUtils::getThemeColor(themeManager, "ui.accent");

  viewport()->setStyleSheet(UiUtils::getScrollBarStylesheet(
      themeManager, "QWidget", backgroundColor,
      QString("border-bottom: 1px solid %1")
          .arg(UiUtils::getThemeColor(themeManager, "ui.border"))));

  dropIndicator->setStyleSheet(
      QString("background-color: %1;").arg(indicatorColor));

  for (auto *tab : tabs) {
    if (tab) {
      tab->update();
    }
  }
}

void TabBarWidget::setTabs(QStringList titles, QStringList paths,
                           rust::Vec<bool> modifiedStates,
                           rust::Vec<bool> pinnedStates) {
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
        titles[i], paths[i], i, pinnedStates[i], configManager, themeManager,
        contextMenuRegistry, commandRegistry,
        [this]() -> int { return tabController->getTabCount(); }, this);
    tabWidget->setModified(modifiedStates[i]);
    tabWidget->setIsPinned(pinnedStates[i]);

    connect(tabWidget, &TabWidget::clicked, this, [this, i]() {
      setCurrentIndex(i);
      emit currentChanged(i);
    });
    connect(tabWidget, &TabWidget::closeRequested, this,
            [this, i, numberOfTitles](bool bypassConfirmation) {
              emit tabCloseRequested(i, numberOfTitles, bypassConfirmation);
            });
    connect(tabWidget, &TabWidget::unpinRequested, this,
            [this, i]() { emit tabUnpinRequested(i); });
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

void TabBarWidget::setTabModified(int index, bool modified) {
  tabs[index]->setModified(modified);
}

int TabBarWidget::getNumberOfTabs() { return tabs.size(); }

void TabBarWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasFormat("application/x-neko-tab-index")) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else {
    event->ignore();
  }
}

void TabBarWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasFormat("application/x-neko-tab-index")) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
    const QPoint viewportPos = event->position().toPoint();
    const QPoint containerPos =
        containerWidget->mapFrom(viewport(), viewportPos);
    int toIndex = dropIndexForPosition(containerPos);

    int pinnedCount = 0;
    for (auto *tab : tabs) {
      if (tab && tab->getIsPinned()) {
        pinnedCount += 1;
      }
    }

    bool ok = false;
    const int fromIndex =
        event->mimeData()->data("application/x-neko-tab-index").toInt(&ok);
    if (ok && fromIndex >= 0 && fromIndex < tabs.size()) {
      const bool draggingPinned = tabs[fromIndex]->getIsPinned();
      if (draggingPinned) {
        if (toIndex > pinnedCount) {
          toIndex = pinnedCount;
        }
      } else {
        if (toIndex < pinnedCount) {
          toIndex = pinnedCount;
        }
      }
    }

    updateDropIndicator(toIndex);
  } else {
    event->ignore();
  }
}

void TabBarWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  dropIndicator->setVisible(false);
  QScrollArea::dragLeaveEvent(event);
}

void TabBarWidget::dropEvent(QDropEvent *event) {
  if (!event->mimeData()->hasFormat("application/x-neko-tab-index")) {
    event->ignore();
    return;
  }

  const QByteArray data =
      event->mimeData()->data("application/x-neko-tab-index");
  bool ok = false;
  const int fromIndex = data.toInt(&ok);

  if (!ok || fromIndex < 0 || fromIndex >= tabs.size()) {
    event->ignore();
    return;
  }

  const QPoint viewportPos = event->position().toPoint();
  const QPoint containerPos = containerWidget->mapFrom(viewport(), viewportPos);
  int toIndex = dropIndexForPosition(containerPos);

  int pinnedCount = 0;

  for (auto *tab : tabs) {
    if (tab && tab->getIsPinned()) {
      pinnedCount += 1;
    }
  }

  const bool draggingPinned = tabs[fromIndex]->getIsPinned();
  if (draggingPinned) {
    if (toIndex > pinnedCount) {
      toIndex = pinnedCount;
    }
  } else {
    if (toIndex < pinnedCount) {
      toIndex = pinnedCount;
    }
  }

  if (fromIndex == toIndex || fromIndex + 1 == toIndex) {
    dropIndicator->setVisible(false);
    event->ignore();
    return;
  }

  tabController->moveTab(fromIndex, toIndex);
  dropIndicator->setVisible(false);
  event->setDropAction(Qt::MoveAction);
  event->accept();
}

bool TabBarWidget::eventFilter(QObject *watched, QEvent *event) {
  if (watched == viewport()) {
    if (event->type() == QEvent::DragEnter) {
      dragEnterEvent(static_cast<QDragEnterEvent *>(event));
      return event->isAccepted();
    }
    if (event->type() == QEvent::DragMove) {
      dragMoveEvent(static_cast<QDragMoveEvent *>(event));
      return event->isAccepted();
    }
    if (event->type() == QEvent::DragLeave) {
      dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
      return event->isAccepted();
    }
    if (event->type() == QEvent::Drop) {
      dropEvent(static_cast<QDropEvent *>(event));
      return event->isAccepted();
    }
  }

  return QScrollArea::eventFilter(watched, event);
}

int TabBarWidget::dropIndexForPosition(const QPoint &pos) const {
  for (int i = 0; i < tabs.size(); i++) {
    const QRect tabRect = tabs[i]->geometry();

    if (pos.x() < tabRect.center().x()) {
      return i;
    }
  }

  return tabs.size();
}

void TabBarWidget::updateDropIndicator(int index) {
  if (tabs.isEmpty()) {
    dropIndicator->setVisible(false);
    return;
  }

  int x = 0;
  int height = containerWidget->height();
  if (index <= 0) {
    x = tabs.first()->geometry().left();
  } else if (index >= tabs.size()) {
    x = tabs.last()->geometry().right() + 1;
  } else {
    x = tabs[index]->geometry().left();
  }

  dropIndicator->setGeometry(x - 1, 0, 2, height);
  dropIndicator->setVisible(true);
  dropIndicator->raise();
}

void TabBarWidget::updateTabAppearance() {
  for (int i = 0; i < tabs.size(); i++) {
    tabs[i]->setActive(i == currentTabIndex);
  }
}

void TabBarWidget::registerCommands() {
  // TODO: Centralize these
  commandRegistry.registerCommand("tab.copyPath", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    QGuiApplication::clipboard()->setText(ctx.filePath);
  });
  commandRegistry.registerCommand("tab.pin", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();

    tabController->setActiveTabIndex(ctx.tabIndex);
    if (ctx.isPinned) {
      tabController->unpinTab(ctx.tabIndex);
    } else {
      tabController->pinTab(ctx.tabIndex);
    }

    emit tabPinnedChanged(tabController->getActiveTabIndex());
  });
}

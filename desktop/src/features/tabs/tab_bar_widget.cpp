#include "tab_bar_widget.h"

#include <algorithm>

// TODO(scarlet): Rework the tab update system to not rely on mass setting
// all the tabs and have the TabBarWidget be in charge of mgmt/updates,
// with each TabWidget in control of its repaints?
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

  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight =
      static_cast<int>(fontMetrics.height() + TOP_PADDING + BOTTOM_PADDING);
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

  currentTabId = 0;
  registerCommands();
  viewport()->update();

  applyTheme();
}

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
    if (tab != nullptr) {
      tab->update();
    }
  }
}

// NOLINTNEXTLINE
void TabBarWidget::setTabs(const QStringList &titles, const QStringList &paths,
                           // NOLINTNEXTLINE
                           const QList<bool> &modifiedStates,
                           const QList<bool> &pinnedStates) {
  QLayoutItem *item;
  while ((item = layout->takeAt(0)) != nullptr) {
    if ((item->widget() != nullptr) && item->widget() != newTabButton) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  tabs.clear();

  // Create new tabs
  auto snapshot = tabController->getTabsSnapshot();
  int tabIndex = 0;
  for (const auto &tab : snapshot.tabs) {
    const int tabId = static_cast<int>(tab.id);
    const QString title = QString::fromUtf8(tab.title);
    const QString path = QString::fromUtf8(tab.path);
    const bool pinned = tab.pinned;
    const bool modified = tab.modified;

    auto *tabWidget = new TabWidget(
        title, path, tabIndex, tabId, pinned, configManager, themeManager,
        contextMenuRegistry, commandRegistry,
        [this]() -> int {
          const auto snapshot = tabController->getTabsSnapshot();
          return static_cast<int>(snapshot.tabs.size());
        },
        this);
    tabWidget->setModified(modified);
    tabWidget->setIsPinned(pinned);

    connect(tabWidget, &TabWidget::clicked, this, [this, tabId]() {
      setCurrentId(tabId);
      emit currentChanged(tabId);
    });
    connect(tabWidget, &TabWidget::closeRequested, this,
            [this, tabId](bool bypassConfirmation) {
              emit tabCloseRequested(tabId, bypassConfirmation);
            });
    connect(tabWidget, &TabWidget::unpinRequested, this,
            [this, tabId]() { emit tabUnpinRequested(tabId); });

    layout->addWidget(tabWidget);
    tabs.append(tabWidget);

    tabIndex++;
  }

  layout->addStretch();
  updateTabAppearance();
}

void TabBarWidget::setCurrentId(int tabId) {
  currentTabId = tabId;
  updateTabAppearance();
}

void TabBarWidget::setTabModified(int tabId, bool modified) {
  auto foundTabWidget =
      std::find_if(tabs.begin(), tabs.end(), [&](TabWidget *tabWidget) {
        return tabWidget && tabWidget->getId() == tabId;
      });

  if (foundTabWidget != tabs.end() && (*foundTabWidget != nullptr)) {
    (*foundTabWidget)->setModified(modified);
  }
}

int TabBarWidget::getNumberOfTabs() { return static_cast<int>(tabs.size()); }

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
      if ((tab != nullptr) && tab->getIsPinned()) {
        pinnedCount += 1;
      }
    }

    bool success = false;
    const int fromIndex =
        event->mimeData()->data("application/x-neko-tab-index").toInt(&success);
    if (success && fromIndex >= 0 && fromIndex < tabs.size()) {
      const bool draggingPinned = tabs[fromIndex]->getIsPinned();
      if (draggingPinned) {
        toIndex = std::min(toIndex, pinnedCount);
      } else {
        toIndex = std::max(toIndex, pinnedCount);
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
  bool success = false;
  const int fromIndex = data.toInt(&success);

  if (!success || fromIndex < 0 || fromIndex >= tabs.size()) {
    event->ignore();
    return;
  }

  const QPoint viewportPos = event->position().toPoint();
  const QPoint containerPos = containerWidget->mapFrom(viewport(), viewportPos);
  int toIndex = dropIndexForPosition(containerPos);

  int pinnedCount = 0;

  for (auto *tab : tabs) {
    if ((tab != nullptr) && tab->getIsPinned()) {
      pinnedCount += 1;
    }
  }

  const bool draggingPinned = tabs[fromIndex]->getIsPinned();
  if (draggingPinned) {
    toIndex = std::min(toIndex, pinnedCount);
  } else {
    toIndex = std::max(toIndex, pinnedCount);
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

  return static_cast<int>(tabs.size());
}

void TabBarWidget::updateDropIndicator(int index) {
  if (tabs.isEmpty()) {
    dropIndicator->setVisible(false);
    return;
  }

  int xPos = 0;
  int height = containerWidget->height();
  if (index <= 0) {
    xPos = tabs.first()->geometry().left();
  } else if (index >= tabs.size()) {
    xPos = tabs.last()->geometry().right() + 1;
  } else {
    xPos = tabs[index]->geometry().left();
  }

  dropIndicator->setGeometry(xPos - 1, 0, 2, height);
  dropIndicator->setVisible(true);
  dropIndicator->raise();
}

void TabBarWidget::updateTabAppearance() {
  const auto snapshot = tabController->getTabsSnapshot();

  int index = 0;
  for (const auto &tab : snapshot.tabs) {
    tabs[index]->setActive(tab.id == currentTabId);
    index++;
  }
}

void TabBarWidget::registerCommands() {
  // TODO(scarlet): Centralize these
  commandRegistry.registerCommand(
      "tab.copyPath", [this](const QVariant &variant) {
        auto ctx = variant.value<TabContext>();
        QGuiApplication::clipboard()->setText(ctx.filePath);
      });
  commandRegistry.registerCommand("tab.pin", [this](const QVariant &variant) {
    auto ctx = variant.value<TabContext>();

    tabController->setActiveTab(ctx.tabId);
    if (ctx.isPinned) {
      tabController->unpinTab(ctx.tabId);
    } else {
      tabController->pinTab(ctx.tabId);
    }

    const auto snapshot = tabController->getTabsSnapshot();
    emit tabPinnedChanged(static_cast<int>(snapshot.active_id));
  });
}

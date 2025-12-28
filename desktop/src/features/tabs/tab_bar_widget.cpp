#include "tab_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_widget.h"
#include "utils/ui_utils.h"
#include <QByteArray>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QMimeData>
#include <QPoint>
#include <QRect>
#include <QScrollBar>
#include <QString>
#include <QVariant>
#include <algorithm>

TabBarWidget::TabBarWidget(const TabBarProps &props, QWidget *parent)
    : QScrollArea(parent), font(props.font), tabBarTheme(props.theme),
      tabTheme(props.tabTheme), themeProvider(props.themeProvider),
      contextMenuRegistry(*props.contextMenuRegistry),
      commandRegistry(*props.commandRegistry),
      tabController(props.tabController) {
  setFont(font);

  QFontMetrics fontMetrics(font);
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

  layout->addStretch();

  setAndApplyTheme(tabBarTheme);
}

void TabBarWidget::setAndApplyTheme(const TabBarTheme &newTheme) {
  tabBarTheme = newTheme;

  viewport()->setStyleSheet(UiUtils::getScrollBarStylesheet(
      tabBarTheme.scrollBarTheme.thumbColor,
      tabBarTheme.scrollBarTheme.thumbHoverColor, "QWidget",
      tabBarTheme.backgroundColor,
      QString("border-bottom: 1px solid %1").arg(tabBarTheme.borderColor)));

  dropIndicator->setStyleSheet(
      QString("background-color: %1;").arg(tabBarTheme.indicatorColor));

  viewport()->update();
}

void TabBarWidget::setAndApplyTabTheme(const TabTheme &newTheme) {
  tabTheme = newTheme;

  for (auto *tab : tabs) {
    if (tab != nullptr) {
      tab->setAndApplyTheme(tabTheme);
    }
  }
}

void TabBarWidget::addTab(const TabPresentation &tab, int index) {
  const int stretchIndex = layout->count() - 1;

  if (index < 0 || index > stretchIndex) {
    index = stretchIndex;
  }

  auto *tabWidget = new TabWidget({.title = tab.title,
                                   .path = tab.path,
                                   .index = index,
                                   .tabId = tab.id,
                                   .isPinned = tab.pinned,
                                   .font = font,
                                   .themeProvider = themeProvider,
                                   .theme = tabTheme,
                                   .contextMenuRegistry = &contextMenuRegistry,
                                   .commandRegistry = &commandRegistry},
                                  this);

  connect(tabWidget, &TabWidget::clicked, this, [this](int tabId) {
    setCurrentTabId(tabId);
    emit currentChanged(tabId);
  });
  connect(tabWidget, &TabWidget::closeRequested, this,
          &TabBarWidget::tabCloseRequested);
  connect(tabWidget, &TabWidget::unpinRequested, this,
          &TabBarWidget::tabUnpinRequested);

  layout->insertWidget(index, tabWidget);
  tabs.insert(index, tabWidget);

  for (int i = 0; i < tabs.size(); ++i) {
    tabs[i]->setIndex(i);
  }
}

void TabBarWidget::removeTab(int tabId) {
  auto *maybeTabWidget = findTabWidgetById(tabId);
  if (maybeTabWidget == nullptr) {
    return;
  }

  const int index = static_cast<int>(tabs.indexOf(maybeTabWidget));
  if (index < 0) {
    return;
  }

  tabs.removeAt(index);
  layout->removeWidget(maybeTabWidget);

  maybeTabWidget->deleteLater();

  for (int i = 0; i < tabs.size(); ++i) {
    tabs[i]->setIndex(i);
  }
}

void TabBarWidget::moveTab(int fromIndex, int toIndex) {
  if (fromIndex < 0 || fromIndex >= tabs.size()) {
    return;
  }

  toIndex = std::max(toIndex, 0);
  toIndex = std::min<int>(toIndex, static_cast<int>(tabs.size() - 1));

  if (fromIndex == toIndex) {
    return;
  }

  TabWidget *tabWidget = tabs.takeAt(fromIndex);

  tabs.insert(toIndex, tabWidget);

  layout->removeWidget(tabWidget);
  layout->insertWidget(toIndex, tabWidget);

  for (int i = 0; i < tabs.size(); ++i) {
    tabs[i]->setIndex(i);
  }
}

void TabBarWidget::updateTab(const TabPresentation &tab) {
  auto *maybeTabWidget = findTabWidgetById(tab.id);
  if (maybeTabWidget == nullptr) {
    return;
  }

  auto updateIfDifferent = [maybeTabWidget](auto getter, auto setter,
                                            const auto &newValue) {
    if ((maybeTabWidget->*getter)() != newValue) {
      (maybeTabWidget->*setter)(newValue);
    }
  };

  updateIfDifferent(&TabWidget::getIsModified, &TabWidget::setModified,
                    tab.modified);
  updateIfDifferent(&TabWidget::getIsPinned, &TabWidget::setIsPinned,
                    tab.pinned);
  updateIfDifferent(&TabWidget::getPath, &TabWidget::setPath, tab.path);
  updateIfDifferent(&TabWidget::getTitle, &TabWidget::setTitle, tab.title);
}

void TabBarWidget::setCurrentTabId(int tabId) {
  currentTabId = tabId;

  for (auto *tab : tabs) {
    if (tab == nullptr) {
      continue;
    }

    tab->setActive(tab->getId() == currentTabId);
  }
}

TabWidget *TabBarWidget::findTabWidgetById(int tabId) {
  auto foundTabWidget =
      std::find_if(tabs.begin(), tabs.end(), [&](TabWidget *tabWidget) {
        return tabWidget && tabWidget->getId() == tabId;
      });

  if (foundTabWidget != tabs.end() && (*foundTabWidget != nullptr)) {
    return *foundTabWidget;
  }

  return nullptr;
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
  int slotIndex = dropIndexForPosition(containerPos);

  int pinnedCount = 0;
  for (auto *tab : tabs) {
    if ((tab != nullptr) && tab->getIsPinned()) {
      ++pinnedCount;
    }
  }

  const bool draggingPinned = tabs[fromIndex]->getIsPinned();
  if (draggingPinned) {
    slotIndex = std::min(slotIndex, pinnedCount);
  } else {
    slotIndex = std::max(slotIndex, pinnedCount);
  }

  int toIndex = slotIndex;
  if (fromIndex < slotIndex) {
    toIndex -= 1;
  }

  toIndex = std::max(toIndex, 0);
  toIndex = std::min(toIndex, static_cast<int>(tabs.size()) - 1);

  if (fromIndex == toIndex) {
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

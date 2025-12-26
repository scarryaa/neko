#include "main_window_layout_builder.h"
#include "features/main_window/controllers/app_config_service.h"
#include "features/main_window/controllers/ui_style_manager.h"
#include "theme/theme_provider.h"
#include "utils/ui_utils.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

namespace k {
constexpr int topTabBarPadding = 8;
constexpr int bottomTabBarPadding = 8;
constexpr int emptyStateNewTabButtonWidth = 80;
constexpr int emptyStateNewTabButtonHeight = 32;
constexpr int splitterLargeWidth = 1000000.0;
} // namespace k

MainWindowLayoutBuilder::MainWindowLayoutBuilder(
    const MainWindowLayoutProps &props, QWidget *parent)
    : props(props), rootParent(parent) {}

MainWindowLayoutBuilder::MainWindowLayoutResult
MainWindowLayoutBuilder::build(const MainWindowLayoutWidgets &widgets) {
  auto *mainContainer = new QWidget(rootParent);
  auto *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(widgets.titleBarWidget);

  // Editor side
  auto *editorSideContainer = new QWidget(rootParent);
  auto *editorSideLayout = new QVBoxLayout(editorSideContainer);
  editorSideLayout->setContentsMargins(0, 0, 0, 0);
  editorSideLayout->setSpacing(0);

  auto tabBarSectionResult = buildTabBarSection(widgets.tabBarWidget);
  editorSideLayout->addWidget(tabBarSectionResult.tabBarContainer);

  auto emptyStateSectionResult = buildEmptyStateSection();
  editorSideLayout->addWidget(
      buildEditorSection(widgets.gutterWidget, widgets.editorWidget,
                         emptyStateSectionResult.emptyStateWidget));

  auto *splitter =
      buildSplitter(editorSideContainer, widgets.fileExplorerWidget);
  mainLayout->addWidget(splitter);
  mainLayout->addWidget(widgets.statusBarWidget);

  MainWindowLayoutResult result{
      .centralWidget = mainContainer,
      .tabBarContainer = tabBarSectionResult.tabBarContainer,
      .newTabButton = tabBarSectionResult.newTabButton,
      .emptyStateWidget = emptyStateSectionResult.emptyStateWidget,
      .emptyStateNewTabButton = emptyStateSectionResult.newTabButton,
      .mainSplitter = splitter};
  return result;
}

MainWindowLayoutBuilder::TabBarSectionResult
MainWindowLayoutBuilder::buildTabBarSection(QWidget *tabBarWidget) {
  auto *tabBarContainer = new QWidget(rootParent);
  auto *tabBarLayout = new QHBoxLayout(tabBarContainer);
  tabBarLayout->setContentsMargins(0, 0, 0, 0);
  tabBarLayout->setSpacing(0);

  auto *newTabButton = new QPushButton("+", tabBarContainer);

  auto uiFont = props.uiStyleManager->interfaceFont();
  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight = static_cast<int>(
      fontMetrics.height() + k::topTabBarPadding + k::bottomTabBarPadding);

  newTabButton->setFixedSize(dynamicHeight, dynamicHeight);
  tabBarLayout->addWidget(tabBarWidget);
  tabBarLayout->addWidget(newTabButton);

  TabBarSectionResult result{.tabBarContainer = tabBarContainer,
                             .newTabButton = newTabButton};
  return result;
}

MainWindowLayoutBuilder::EmptyStateSectionResult
MainWindowLayoutBuilder::buildEmptyStateSection() {
  auto *emptyStateWidget = new QWidget(rootParent);
  auto *emptyLayout = new QVBoxLayout(emptyStateWidget);
  emptyLayout->setAlignment(Qt::AlignCenter);

  auto *newTabButton = new QPushButton("New Tab", emptyStateWidget);
  newTabButton->setFixedSize(k::emptyStateNewTabButtonWidth,
                             k::emptyStateNewTabButtonHeight);

  emptyLayout->addWidget(newTabButton);
  emptyStateWidget->hide();

  EmptyStateSectionResult result = {.emptyStateWidget = emptyStateWidget,
                                    .newTabButton = newTabButton};
  return result;
}

QWidget *MainWindowLayoutBuilder::buildEditorSection(QWidget *gutterWidget,
                                                     QWidget *editorWidget,
                                                     QWidget *emptyState) {
  auto *editorContainer = new QWidget(rootParent);
  auto *editorLayout = new QHBoxLayout(editorContainer);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);
  editorLayout->addWidget(gutterWidget, 0);
  editorLayout->addWidget(editorWidget, 1);
  editorLayout->addWidget(emptyState);
  return editorContainer;
}

QSplitter *MainWindowLayoutBuilder::buildSplitter(QWidget *editorSideContainer,
                                                  QWidget *fileExplorerWidget) {
  auto *splitter = new QSplitter(Qt::Horizontal, rootParent);

  auto snapshot = props.appConfigService->getSnapshot();
  auto fileExplorerRight = snapshot.file_explorer_right;
  int savedSidebarWidth = static_cast<int>(snapshot.file_explorer_width);

  if (fileExplorerRight) {
    splitter->addWidget(editorSideContainer);
    splitter->addWidget(fileExplorerWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes(QList<int>()
                       << k::splitterLargeWidth << savedSidebarWidth);
  } else {
    splitter->addWidget(fileExplorerWidget);
    splitter->addWidget(editorSideContainer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>()
                       << savedSidebarWidth << k::splitterLargeWidth);
  }

  splitter->setHandleWidth(1);

  QString handleColor = props.themeProvider->getSplitterTheme().handleColor;
  splitter->setStyleSheet(
      QStringLiteral("QSplitter::handle { background-color: %1; margin: 0px; }")
          .arg(handleColor));

  auto *appConfigService = props.appConfigService;
  QObject::connect(splitter, &QSplitter::splitterMoved, rootParent,
                   [splitter, appConfigService](int pos, int index) {
                     const QList<int> sizes = splitter->sizes();
                     if (sizes.size() < 2) {
                       return;
                     }

                     auto snapshot = appConfigService->getSnapshot();

                     const bool fileExplorerRight =
                         snapshot.file_explorer_right;
                     const int fileExplorerWidth =
                         fileExplorerRight ? sizes[1] : sizes[0];

                     appConfigService->setFileExplorerWidth(fileExplorerWidth);
                   });

  return splitter;
}

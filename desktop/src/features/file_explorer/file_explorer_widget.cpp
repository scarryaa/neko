#include "file_explorer_widget.h"
#include "utils/gui_utils.h"

FileExplorerWidget::FileExplorerWidget(neko::FileTree *tree,
                                       neko::ConfigManager &configManager,
                                       neko::ThemeManager &themeManager,
                                       QWidget *parent)
    : QScrollArea(parent), tree(tree), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::FileExplorer)),
      fontMetrics(font) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  auto backgroundColor =
      UiUtils::getThemeColor(themeManager, "file_explorer.background");
  setStyleSheet(
      UiUtils::getScrollBarStylesheet("FileExplorerWidget", backgroundColor));

  directorySelectionButton = new QPushButton("Select a directory");
  directorySelectionButton->setStyleSheet(
      "QPushButton { background-color: #202020; color: #e0e0e0; border-radius: "
      "6px; padding: 8px 16px; font-size: 13px; }"
      "QPushButton:hover { background-color: #444444; border-color: #555555; }"
      "QPushButton:pressed { background-color: #222222; border-color: #333333; "
      "}");

  auto layout = new QVBoxLayout();
  layout->addWidget(directorySelectionButton, 0, Qt::AlignCenter);
  setLayout(layout);

  connect(directorySelectionButton, &QPushButton::clicked, this,
          [this]() { directorySelectionRequested(); });
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { redraw(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { redraw(); });
}

FileExplorerWidget::~FileExplorerWidget() {}

void FileExplorerWidget::focusInEvent(QFocusEvent *event) {
  focusReceivedFromMouse = event->reason() == Qt::MouseFocusReason;
  QScrollArea::focusInEvent(event);
}

void FileExplorerWidget::focusOutEvent(QFocusEvent *event) {
  focusReceivedFromMouse = false;
  QScrollArea::focusOutEvent(event);
}

void FileExplorerWidget::resizeEvent(QResizeEvent *event) {
  updateDimensions();
}

void FileExplorerWidget::loadSavedDir() {
  auto rawPath = configManager.get_file_explorer_directory();
  QString savedDir = QString::fromUtf8(rawPath);

  if (!savedDir.isEmpty()) {
    initialize(savedDir.toStdString());
    rootPath = savedDir.toStdString();
    directorySelectionButton->hide();
  }
}

void FileExplorerWidget::directorySelectionRequested() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select a directory", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!dir.isEmpty()) {
    initialize(dir.toStdString());
    rootPath = dir.toStdString();
    directorySelectionButton->hide();
    configManager.set_file_explorer_directory(dir.toStdString());
  }
}

void FileExplorerWidget::initialize(std::string path) {
  tree->set_root_dir(path);
  emit directorySelected(path);
  loadDirectory(path);
}

void FileExplorerWidget::loadDirectory(const std::string path) {
  setFileNodes(tree->get_children(path));
  redraw();
}

void FileExplorerWidget::setFileNodes(rust::Vec<neko::FileNode> nodes,
                                      bool updateScrollbars) {
  fileNodes = std::move(nodes);
  fileCount = fileNodes.size();

  if (updateScrollbars) {
    updateDimensions();
  }
}

void FileExplorerWidget::refreshVisibleNodes(bool updateScrollbars) {
  setFileNodes(tree->get_visible_nodes(), updateScrollbars);
}

double FileExplorerWidget::measureContent() {
  double finalWidth = 0;

  // TODO: Make this faster
  for (int i = 0; i < fileCount; i++) {
    auto rawLine = fileNodes[i].name;
    QString lineText = QString::fromUtf8(rawLine);

    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::updateDimensions() {
  const double lineHeight = fontMetrics.height();
  const double viewportHeight = std::max(
      0.0, (fileCount * lineHeight) - viewport()->height() + VIEWPORT_PADDING);
  const double viewportWidth =
      std::max(0.0, measureContent() - viewport()->width() + VIEWPORT_PADDING);

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);
}

void FileExplorerWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  double horizontallDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().x() / 4.0;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(newHorizontalScrollOffset);
  verticalScrollBar()->setValue(newVerticalScrollOffset);
  redraw();
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  const bool refocusClick = focusReceivedFromMouse;
  focusReceivedFromMouse = false;

  int row = convertMousePositionToRow(event->pos().y());

  if (row >= fileCount) {
    if (refocusClick) {
      setFocus();
      return;
    }

    tree->clear_current();
    redraw();
    return;
  }

  auto node = fileNodes[row];
  tree->set_current(node.path);

  if (node.is_dir) {
    tree->toggle_expanded(node.path);
    refreshVisibleNodes();
  } else {
    QString fileStr = QString::fromUtf8(node.path);
    emit fileSelected(fileStr.toStdString(), false);
  }

  redraw();
}

int FileExplorerWidget::convertMousePositionToRow(double y) {
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();

  const size_t targetRow = (y + scrollY) / lineHeight;

  return targetRow;
}

void FileExplorerWidget::keyPressEvent(QKeyEvent *event) {
  bool shouldScroll = false;

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);

  if (ctrl) {
    switch (event->key()) {
    case Qt::Key_C:
      handleCopy();
      return;
    case Qt::Key_V:
      handlePaste();
      return;
    case Qt::Key_Equal:
      increaseFontSize();
      return;
    case Qt::Key_Minus:
      decreaseFontSize();
      return;
    case Qt::Key_0:
      resetFontSize();
      return;
    }
  }

  switch (event->key()) {
  case Qt::Key_Up:
    selectPrevNode();
    shouldScroll = true;
    break;
  case Qt::Key_Down:
    selectNextNode();
    shouldScroll = true;
    break;
  case Qt::Key_Left:
    handleLeft();
    shouldScroll = true;
    break;
  case Qt::Key_Right:
    handleRight();
    shouldScroll = true;
    break;
  case Qt::Key_Space:
    toggleSelectNode();
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    handleEnter();
    break;
  case Qt::Key_Delete:
    if (shift) {
      handleDeleteNoConfirm();
    } else {
      handleDeleteConfirm();
    }
  }

  if (shouldScroll) {
    int index = tree->get_current_index();
    scrollToNode(index);
  }

  redraw();
}

void FileExplorerWidget::redraw() { viewport()->update(); }

void FileExplorerWidget::resetFontSize() { setFontSize(DEFAULT_FONT_SIZE); }

void FileExplorerWidget::increaseFontSize() {
  if (font.pointSizeF() < FONT_UPPER_LIMIT) {
    setFontSize(font.pointSizeF() + FONT_STEP);
  }
}

void FileExplorerWidget::decreaseFontSize() {
  if (font.pointSizeF() > FONT_LOWER_LIMIT) {
    setFontSize(font.pointSizeF() - FONT_STEP);
  }
}

void FileExplorerWidget::setFontSize(double newFontSize) {
  font.setPointSizeF(newFontSize);
  fontMetrics = QFontMetricsF(font);

  redraw();
  UiUtils::setFontSize(configManager, neko::FontType::FileExplorer,
                       newFontSize);
}

void FileExplorerWidget::handleCopy() {
  auto rawCurrentPath = tree->get_path_of_current();

  QMimeData *mimeData = new QMimeData();

  QList<QUrl> urls;
  urls.append(QUrl::fromLocalFile(QString::fromUtf8(rawCurrentPath)));
  mimeData->setUrls(urls);

  QApplication::clipboard()->setMimeData(mimeData);
}

void FileExplorerWidget::handleDeleteNoConfirm() {
  auto rawCurrentPath = tree->get_path_of_current();
  auto currentNode = tree->get_node(rawCurrentPath);

  deleteItem(rawCurrentPath.c_str(), currentNode);
}

void FileExplorerWidget::handleDeleteConfirm() {
  auto rawCurrentPath = tree->get_path_of_current();
  auto currentNode = tree->get_node(rawCurrentPath);

  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      this, "Delete Item",
      "Are you sure you want to delete " + currentNode.name +
          "?\n\n(Hold shift to bypass this dialog)",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

  if (reply == QMessageBox::Yes) {
    deleteItem(rawCurrentPath.c_str(), currentNode);
  }
}

void FileExplorerWidget::deleteItem(std::string path,
                                    neko::FileNode currentNode) {
  auto prevNode = tree->get_prev_node(path);
  auto parentPath = tree->get_path_of_parent(path);
  bool currentIsDir = currentNode.is_dir;

  if (currentIsDir) {
    QDir dir = QDir(QString::fromStdString(path));

    if (dir.removeRecursively()) {
      tree->refresh_dir(parentPath);
      refreshVisibleNodes();

      tree->set_current(prevNode.path);
    } else {
      qDebug() << "Failed to remove directory";
    }
  } else {
    QFile file = QFile(QString::fromStdString(path));

    if (file.remove()) {
      tree->refresh_dir(parentPath);
      refreshVisibleNodes();

      tree->set_current(prevNode.path);
    } else {
      qDebug() << "Failed to remove file";
    }
  }
}

void FileExplorerWidget::handlePaste() {
  auto rawCurrentPath = tree->get_path_of_current();
  auto parentPath = tree->get_path_of_parent(rawCurrentPath);
  auto currentNode = tree->get_node(rawCurrentPath);
  bool currentIsDir = currentNode.is_dir;

  QString targetDirectory = currentIsDir ? QString::fromUtf8(rawCurrentPath)
                                         : QString::fromUtf8(parentPath);

  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  if (!mimeData->hasUrls())
    return;

  QList<QUrl> urls = mimeData->urls();

  for (const auto &url : urls) {
    QString srcPath = url.toLocalFile();
    QFileInfo srcInfo(srcPath);

    QString destPath = targetDirectory + QDir::separator() + srcInfo.fileName();

    if (destPath.startsWith(srcPath)) {
      continue;
    }

    if (srcInfo.isDir()) {
      copyRecursively(srcPath, destPath);
    } else {
      if (QFile::exists(destPath)) {
        // QFile::remove(destPath);
        qDebug() << "File already exists:" << destPath;
      } else {
        if (!QFile::copy(srcPath, destPath)) {
          qDebug() << "Failed to copy file:" << srcPath;
        }
      }
    }
  }

  tree->refresh_dir(currentIsDir ? rawCurrentPath : parentPath);
  refreshVisibleNodes();
}

bool FileExplorerWidget::copyRecursively(QString sourceFolder,
                                         QString destFolder) {
  QDir sourceDir(sourceFolder);
  if (!sourceDir.exists())
    return false;

  QDir destDir(destFolder);
  if (!destDir.exists()) {
    destDir.mkpath(".");
  }

  QStringList files = sourceDir.entryList(QDir::Files | QDir::Dirs |
                                          QDir::NoDotAndDotDot | QDir::Hidden);

  for (const QString &file : files) {
    QString srcName = sourceFolder + QDir::separator() + file;
    QString destName = destFolder + QDir::separator() + file;

    QFileInfo info(srcName);
    if (info.isDir()) {
      copyRecursively(srcName, destName);
    } else {
      QFile::copy(srcName, destName);
    }
  }
  return true;
}

void FileExplorerWidget::handleLeft() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is collapsed, go to parent and collapse
  if (!tree->is_expanded(rawCurrentPath)) {
    auto parent = tree->get_path_of_parent(rawCurrentPath);
    if (parent != rootPath) {
      tree->set_current(parent);
      tree->set_collapsed(parent);
    }
  } else {
    // Otherwise, collapse focused node and stay put
    collapseNode();
  }

  refreshVisibleNodes();
}

void FileExplorerWidget::handleRight() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is collapsed, expand and stay put
  if (!tree->is_expanded(rawCurrentPath)) {
    expandNode();
  } else {
    // Otherwise, go to first child
    auto children = tree->get_children(rawCurrentPath);

    if (!children.empty()) {
      tree->set_current(children[0].path);
    }
  }

  refreshVisibleNodes();
}

void FileExplorerWidget::handleEnter() {
  auto rawCurrentPath = tree->get_path_of_current();

  // If focused node is a directory, toggle expansion
  auto currentFile = tree->get_node(rawCurrentPath);
  bool isDir = currentFile.is_dir;

  if (!rawCurrentPath.empty()) {
    if (isDir) {
      if (!tree->is_expanded(rawCurrentPath)) {
        expandNode();
      } else {
        collapseNode();
      }
    } else {
      // Otherwise, open the file in the editor
      emit fileSelected(rawCurrentPath.c_str());
    }
  } else {
    if (!fileNodes.empty()) {
      tree->set_current(fileNodes[0].path);
    } else {
      return;
    }
  }

  refreshVisibleNodes();
}

void FileExplorerWidget::selectNextNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (!fileNodes.empty()) {
      tree->set_current(fileNodes[0].path);
    } else {
      return;
    }
    return;
  }

  if (tree->get_current_index() == fileNodes.size() - 1) {
    // Wrap to start
    tree->set_current(fileNodes.begin()->path);
    return;
  }

  auto next = tree->get_next_node(rawCurrentPath);
  tree->set_current(next.path);
}

void FileExplorerWidget::selectPrevNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  if (tree->get_current_index() == 0) {
    // Wrap to end
    tree->set_current(fileNodes.at(fileNodes.size() - 1).path);
    return;
  }

  auto prev = tree->get_prev_node(rawCurrentPath);
  tree->set_current(prev.path);
}

void FileExplorerWidget::toggleSelectNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->toggle_select(rawCurrentPath);
}

void FileExplorerWidget::expandNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->set_expanded(rawCurrentPath);
  refreshVisibleNodes();
}

void FileExplorerWidget::collapseNode() {
  auto rawCurrentPath = tree->get_path_of_current();

  if (rawCurrentPath.empty()) {
    if (fileNodes.empty()) {
      return;
    }

    tree->set_current(fileNodes[0].path);
    return;
  }

  tree->set_collapsed(rawCurrentPath);
  refreshVisibleNodes();
}

void FileExplorerWidget::scrollToNode(int index) {
  const double viewportHeight = viewport()->height();
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();
  const size_t nodeY = (index * lineHeight);

  if (nodeY + lineHeight > viewportHeight - VIEWPORT_PADDING + scrollY) {
    verticalScrollBar()->setValue(nodeY - viewportHeight + VIEWPORT_PADDING +
                                  lineHeight);
  } else if (nodeY < scrollY + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(nodeY - VIEWPORT_PADDING);
  }
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());

  drawFiles(&painter, fileCount, fileNodes);
}

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   rust::Vec<neko::FileNode> nodes) {
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();

  int startRow = std::floor(verticalOffset / lineHeight);
  int endRow = std::ceil((verticalOffset + viewportHeight) / lineHeight);

  startRow = std::max(0, startRow);
  endRow = std::min((int)count, endRow);

  for (int i = startRow; i < endRow; i++) {
    double y = (i * lineHeight) - verticalOffset;
    drawFile(painter, -horizontalOffset + ICON_EDGE_PADDING, y, nodes[i]);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double x, double y,
                                  neko::FileNode node) {
  painter->setFont(font);

  double indent = node.depth * 20.0;
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();
  double horizontalOffset = horizontalScrollBar()->value();

  bool isSelected = tree->is_selected(node.path);
  bool isCurrent = tree->is_current(node.path);

  auto accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  QColor selectionColor = QColor(accentColor);
  selectionColor.setAlpha(60);

  // Selection background
  if (isSelected) {
    painter->setBrush(selectionColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRectF(
        -horizontalOffset, y,
        viewportWidth + ICON_EDGE_PADDING + horizontalOffset, lineHeight));
  }

  // Current item border
  if (isCurrent && hasFocus()) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(accentColor);
    painter->drawRect(QRectF(-horizontalOffset, y,
                             viewportWidth - 1 + horizontalOffset,
                             lineHeight - 1));
  }

  // Get appropriate icon
  QIcon icon;
  if (node.is_dir) {
    bool isExpanded = tree->is_expanded(node.path);

    icon = QApplication::style()->standardIcon(
        isExpanded ? QStyle::SP_DirOpenIcon : QStyle::SP_DirIcon);
  } else {
    icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
  }

  int iconSize = lineHeight - ICON_ADJUSTMENT;
  QIcon colorizedIcon = UiUtils::createColorizedIcon(icon, accentColor,
                                                     QSize(iconSize, iconSize));
  QPixmap pixmap = colorizedIcon.pixmap(iconSize, iconSize);

  if (node.is_hidden) {
    pixmap = colorizedIcon.pixmap(iconSize, iconSize, QIcon::Mode::Disabled,
                                  QIcon::State::Off);
  }

  if (!node.is_dir) {
    pixmap = icon.pixmap(iconSize, iconSize);
  }

  // Draw icon with indentation
  double iconX = x + indent + 2;
  double iconY = y + (lineHeight - iconSize) / 2;
  painter->drawPixmap(QPointF(iconX, iconY), pixmap);

  // Draw text after icon
  double textX = iconX + iconSize + 4;
  if (node.is_hidden) {
    QString foregroundMuted =
        UiUtils::getThemeColor(themeManager, "ui.foreground.muted");
    painter->setBrush(foregroundMuted);
    painter->setPen(foregroundMuted);
  } else {
    painter->setBrush(Qt::white);
    painter->setPen(Qt::white);
  }
  painter->drawText(QPointF(textX, y + fontMetrics.ascent()),
                    QString::fromUtf8(node.name));
}

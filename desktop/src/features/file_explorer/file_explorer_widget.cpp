#include "file_explorer_widget.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtDebug>

FileExplorerWidget::FileExplorerWidget(FileTreeController *fileTreeController,
                                       neko::ConfigManager &configManager,
                                       const FileExplorerTheme &theme,
                                       QWidget *parent)
    : QScrollArea(parent), fileTreeController(fileTreeController),
      configManager(configManager), theme(theme),
      font(UiUtils::loadFont(configManager, neko::FontType::FileExplorer)),
      fontMetrics(font) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  directorySelectionButton = new QPushButton("Select a directory");

  setAndApplyTheme(theme);

  auto *layout = new QVBoxLayout();
  layout->addWidget(directorySelectionButton, 0, Qt::AlignCenter);
  setLayout(layout);

  connectSignals();
}

void FileExplorerWidget::connectSignals() {
  connect(directorySelectionButton, &QPushButton::clicked, this,
          &FileExplorerWidget::directorySelectionRequested);
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          &FileExplorerWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &FileExplorerWidget::redraw);
}

void FileExplorerWidget::initialize(const std::string &path) {
  fileTreeController->setRootDir(path);
  emit directorySelected(path);
  loadDirectory(path);
}

void FileExplorerWidget::loadSavedDir() {
  auto snapshot = configManager.get_config_snapshot();
  auto rawPath = snapshot.file_explorer_directory;
  bool maybeRoot = snapshot.file_explorer_directory_present;
  QString savedDir = QString::fromUtf8(rawPath);

  if (!savedDir.isEmpty() && maybeRoot) {
    initialize(savedDir.toStdString());
    directorySelectionButton->hide();
  }
}

void FileExplorerWidget::setAndApplyTheme(const FileExplorerTheme &newTheme) {
  theme = newTheme;

  setStyleSheet(UiUtils::getScrollBarStylesheet(
      theme.scrollBarTheme.thumbColor, theme.scrollBarTheme.thumbHoverColor,
      "FileExplorerWidget", theme.backgroundColor));

  if (directorySelectionButton != nullptr) {
    QString buttonBg = theme.buttonBackgroundColor;
    QString buttonHover = theme.buttonHoverColor;
    QString buttonPressed = theme.buttonPressColor;
    QString buttonText = theme.buttonForegroundColor;

    directorySelectionButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border-radius: "
                "6px; padding: 8px 16px; font-size: 15px; border: none; }"
                "QPushButton:hover { background-color: %3; }"
                "QPushButton:pressed { background-color: %4; }")
            .arg(buttonBg, buttonText, buttonHover, buttonPressed));
  }

  redraw();
}

void FileExplorerWidget::showItem(const QString &path) {
  QFileInfo info(path);

  QString parentDir = info.absolutePath();

  std::string itemPath = path.toStdString();
  std::string parentPath = parentDir.toStdString();

  fileTreeController->setExpanded(parentPath);
  fileTreeController->setCurrent(itemPath);
  setFocus();
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
    default:
      break;
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
  default:
    break;
  }

  if (shouldScroll) {
    auto snapshot = fileTreeController->getTreeSnapshot();
    int index = 0;
    for (auto &node : snapshot.nodes) {
      if (node.is_current) {
        break;
      }

      index++;
    }

    scrollToNode(index);
  }

  redraw();
}

void FileExplorerWidget::mousePressEvent(QMouseEvent *event) {
  auto snapshot = fileTreeController->getTreeSnapshot();
  const bool refocusClick = focusReceivedFromMouse;
  focusReceivedFromMouse = false;

  int row = convertMousePositionToRow(event->pos().y());

  if (row >= snapshot.nodes.size()) {
    if (refocusClick) {
      setFocus();
      return;
    }

    fileTreeController->clearCurrent();
    redraw();
    return;
  }

  auto currentNode = snapshot.nodes[row];
  fileTreeController->setCurrent(currentNode.path.c_str());

  if (currentNode.is_dir) {
    fileTreeController->toggleExpanded(currentNode.path.c_str());
  } else {
    QString fileStr = QString::fromUtf8(currentNode.path.c_str());
    emit fileSelected(fileStr.toStdString(), false);
  }

  redraw();
}

void FileExplorerWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
  auto snapshot = fileTreeController->getTreeSnapshot();

  drawFiles(&painter, snapshot.nodes.size(), snapshot.nodes);
}

void FileExplorerWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta = (event->isInverted() ? -1 : 1) *
                         event->angleDelta().y() / SCROLL_WHEEL_DIVIDER;
  double horizontallDelta = (event->isInverted() ? -1 : 1) *
                            event->angleDelta().x() / SCROLL_WHEEL_DIVIDER;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(static_cast<int>(newHorizontalScrollOffset));
  verticalScrollBar()->setValue(static_cast<int>(newVerticalScrollOffset));
  redraw();
}

void FileExplorerWidget::resizeEvent(QResizeEvent *event) {
  updateDimensions();
}

void FileExplorerWidget::focusInEvent(QFocusEvent *event) {
  focusReceivedFromMouse = event->reason() == Qt::MouseFocusReason;
  QScrollArea::focusInEvent(event);
}

void FileExplorerWidget::focusOutEvent(QFocusEvent *event) {
  focusReceivedFromMouse = false;
  QScrollArea::focusOutEvent(event);
}

void FileExplorerWidget::directorySelectionRequested() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select a directory", QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!dir.isEmpty()) {
    initialize(dir.toStdString());
    directorySelectionButton->hide();

    auto snapshot = configManager.get_config_snapshot();
    snapshot.file_explorer_directory_present = true;
    snapshot.file_explorer_directory = dir.toStdString();
    configManager.apply_config_snapshot(snapshot);
  }
}

void FileExplorerWidget::redraw() { viewport()->update(); }

void FileExplorerWidget::drawFiles(QPainter *painter, size_t count,
                                   rust::Vec<neko::FileNodeSnapshot> nodes) {
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();

  int startRow = std::floor(verticalOffset / lineHeight);
  int endRow = std::ceil((verticalOffset + viewportHeight) / lineHeight);

  startRow = std::max(0, startRow);
  endRow = std::min(static_cast<int>(count), endRow);

  for (int i = startRow; i < endRow; i++) {
    double yPos = (i * lineHeight) - verticalOffset;
    drawFile(painter, -horizontalOffset + ICON_EDGE_PADDING, yPos, nodes[i]);
  }
}

void FileExplorerWidget::drawFile(QPainter *painter, double xPos, double yPos,
                                  const neko::FileNodeSnapshot &node) {
  painter->setFont(font);

  double indent = static_cast<int>(node.depth) * NODE_INDENT;
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();
  double horizontalOffset = horizontalScrollBar()->value();

  auto accentColor = theme.selectionColor;
  auto selectionColor = QColor(accentColor);
  selectionColor.setAlpha(SELECTION_ALPHA);

  auto fileTextColor = theme.fileForegroundColor;

  // Selection background
  if (node.is_selected) {
    painter->setBrush(selectionColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRectF(
        -horizontalOffset, yPos,
        viewportWidth + ICON_EDGE_PADDING + horizontalOffset, lineHeight));
  }

  // Current item border
  if (node.is_current && hasFocus()) {
    painter->setBrush(Qt::NoBrush);
    painter->setPen(accentColor);
    painter->drawRect(QRectF(-horizontalOffset, yPos,
                             viewportWidth - 1 + horizontalOffset,
                             lineHeight - 1));
  }

  // Get appropriate icon
  QIcon icon;
  if (node.is_dir) {
    icon = QApplication::style()->standardIcon(
        node.is_expanded ? QStyle::SP_DirOpenIcon : QStyle::SP_DirIcon);
  } else {
    icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
  }

  int iconSize = static_cast<int>(lineHeight - ICON_ADJUSTMENT);
  QIcon colorizedIcon = UiUtils::createColorizedIcon(icon, accentColor,
                                                     QSize(iconSize, iconSize));
  QIcon normalIcon = UiUtils::createColorizedIcon(icon, fileTextColor,
                                                  QSize(iconSize, iconSize));
  QPixmap pixmap = colorizedIcon.pixmap(iconSize, iconSize);

  if (node.is_hidden) {
    pixmap = colorizedIcon.pixmap(iconSize, iconSize, QIcon::Mode::Disabled,
                                  QIcon::State::Off);
  }

  if (!node.is_dir) {
    pixmap = normalIcon.pixmap(iconSize, iconSize);
  }

  // Draw icon with indentation
  double iconX = xPos + indent + 2;
  double iconY = yPos + ((lineHeight - iconSize) / 2);
  painter->drawPixmap(QPointF(iconX, iconY), pixmap);

  // Draw text after icon
  double textX = iconX + iconSize + 4;
  if (node.is_hidden) {
    QString foregroundVeryMuted = theme.fileHiddenColor;
    painter->setBrush(foregroundVeryMuted);
    painter->setPen(foregroundVeryMuted);
  } else {
    painter->setBrush(fileTextColor);
    painter->setPen(fileTextColor);
  }
  painter->drawText(QPointF(textX, yPos + fontMetrics.ascent()),
                    QString::fromUtf8(node.name));
}

void FileExplorerWidget::loadDirectory(const std::string &path) {
  fileTreeController->setExpanded(path);
  updateDimensions();
  redraw();
}

double FileExplorerWidget::measureContent() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  double finalWidth = 0;

  // TODO(scarlet): Make this faster
  for (auto &node : snapshot.nodes) {
    QString lineText = QString::fromUtf8(node.name.c_str());
    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void FileExplorerWidget::updateDimensions() {
  auto snapshot = fileTreeController->getTreeSnapshot();

  const double lineHeight = fontMetrics.height();
  const double viewportHeight =
      std::max(0.0, (static_cast<int>(snapshot.nodes.size()) * lineHeight) -
                        viewport()->height() + VIEWPORT_PADDING);
  const double viewportWidth =
      std::max(0.0, measureContent() - viewport()->width() + VIEWPORT_PADDING);

  horizontalScrollBar()->setRange(0, static_cast<int>(viewportWidth));
  verticalScrollBar()->setRange(0, static_cast<int>(viewportHeight));
}

void FileExplorerWidget::scrollToNode(int index) {
  const double viewportHeight = viewport()->height();
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();
  const double nodeY = index * lineHeight;

  if (nodeY + lineHeight > viewportHeight - VIEWPORT_PADDING + scrollY) {
    verticalScrollBar()->setValue(static_cast<int>(
        nodeY - viewportHeight + VIEWPORT_PADDING + lineHeight));
  } else if (nodeY < scrollY + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(static_cast<int>(nodeY - VIEWPORT_PADDING));
  }
}

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

void FileExplorerWidget::resetFontSize() { setFontSize(DEFAULT_FONT_SIZE); }

void FileExplorerWidget::setFontSize(double newFontSize) {
  font.setPointSizeF(newFontSize);
  fontMetrics = QFontMetricsF(font);

  redraw();
  UiUtils::setFontSize(configManager, neko::FontType::FileExplorer,
                       newFontSize);
}

void FileExplorerWidget::handleEnter() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  neko::FileNodeSnapshot currentNode;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNode = node;
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is a directory, toggle expansion
  bool isDir = currentNode.is_dir;

  if (!currentNodePath.empty()) {
    if (isDir) {
      if (!currentNodeExpanded) {
        expandNode();
      } else {
        collapseNode();
      }
    } else {
      // Otherwise, open the file in the editor
      emit fileSelected(currentNodePath);
    }
  } else {
    // If no node is selected (and the tree is not empty), select the first node
    if (!snapshot.nodes.empty()) {
      fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    } else {
      return;
    }
  }
}

void FileExplorerWidget::handleLeft() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is collapsed, go to parent and collapse
  if (!currentNodeExpanded) {
    auto parentPath = fileTreeController->getParentNodePath(currentNodePath);
    if (parentPath != snapshot.root.c_str()) {
      fileTreeController->setCurrent(parentPath);
      fileTreeController->setCollapsed(parentPath);
    }
  } else {
    // Otherwise, collapse focused node and stay put
    collapseNode();
  }
}

void FileExplorerWidget::handleRight() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  neko::FileNodeSnapshot currentNode;
  bool currentNodeExpanded = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNode = node;
      currentNodeExpanded = node.is_expanded;
      break;
    }
  }

  // If focused node is collapsed, expand and stay put
  if (!currentNodeExpanded) {
    expandNode();
  } else {
    // Otherwise, go to first child
    auto children = fileTreeController->getVisibleChildren(currentNodePath);

    if (!children.empty()) {
      fileTreeController->setCurrent(children[0].path.c_str());
    }
  }
}

void FileExplorerWidget::handleCopy() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
    }
  }

  auto *mimeData = new QMimeData();

  QList<QUrl> urls;
  urls.append(QUrl::fromLocalFile(currentNodePath.c_str()));
  mimeData->setUrls(urls);

  QApplication::clipboard()->setMimeData(mimeData);
}

void FileExplorerWidget::handlePaste() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  bool currentNodeIsDirectory = false;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      currentNodeIsDirectory = node.is_dir;
    }
  }

  auto parentNodePath = fileTreeController->getParentNodePath(currentNodePath);
  QString targetDirectory = currentNodeIsDirectory
                                ? QString::fromUtf8(currentNodePath)
                                : QString::fromUtf8(parentNodePath);

  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  if (!mimeData->hasUrls()) {
    return;
  }

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

  fileTreeController->refreshDirectory(currentNodeIsDirectory ? currentNodePath
                                                              : parentNodePath);
}

bool FileExplorerWidget::copyRecursively(const QString &sourceFolder,
                                         const QString &destFolder) {
  QDir sourceDir(sourceFolder);
  if (!sourceDir.exists()) {
    return false;
  }

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

void FileExplorerWidget::handleDeleteConfirm() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  neko::FileNodeSnapshot currentNode;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNode = node;
      break;
    }
  }

  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      this, "Delete Item",
      "Are you sure you want to delete " + currentNode.name +
          "?\n\n(Hold shift to bypass this dialog)",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

  if (reply == QMessageBox::Yes) {
    deleteItem(currentNode.path.c_str(), currentNode);
  }
}

void FileExplorerWidget::handleDeleteNoConfirm() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  neko::FileNodeSnapshot currentNode;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNode = node;
      break;
    }
  }

  deleteItem(currentNode.path.c_str(), currentNode);
}

void FileExplorerWidget::deleteItem(const std::string &path,
                                    const neko::FileNodeSnapshot &currentNode) {
  auto prevNode = fileTreeController->getPreviousNode(path);
  auto parentPath = fileTreeController->getParentNodePath(path);
  bool currentIsDir = currentNode.is_dir;

  if (currentIsDir) {
    QDir dir = QDir(QString::fromStdString(path));

    if (dir.removeRecursively()) {
      fileTreeController->refreshDirectory(parentPath);

      fileTreeController->setCurrent(prevNode.path.c_str());
    } else {
      qDebug() << "Failed to remove directory";
    }
  } else {
    QFile file = QFile(QString::fromStdString(path));

    if (file.remove()) {
      fileTreeController->refreshDirectory(parentPath);

      fileTreeController->setCurrent(prevNode.path.c_str());
    } else {
      qDebug() << "Failed to remove file";
    }
  }
}

void FileExplorerWidget::selectNextNode() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (!snapshot.nodes.empty()) {
      fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    } else {
      return;
    }
    return;
  }

  // If we are at the bottom
  if (currentNodePath == snapshot.nodes.back().path.c_str()) {
    // Wrap to start
    fileTreeController->setCurrent(snapshot.nodes.begin()->path.c_str());
    return;
  }

  auto next = fileTreeController->getNextNode(currentNodePath);
  fileTreeController->setCurrent(next.path.c_str());
}

void FileExplorerWidget::selectPrevNode() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  // If we are at the top (root node)
  if (currentNodePath == snapshot.nodes[0].path.c_str()) {
    // Wrap to end
    fileTreeController->setCurrent(
        snapshot.nodes.at(snapshot.nodes.size() - 1).path.c_str());
    return;
  }

  auto prev = fileTreeController->getPreviousNode(currentNodePath);
  fileTreeController->setCurrent(prev.path.c_str());
}

void FileExplorerWidget::toggleSelectNode() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeController->toggleSelect(currentNodePath);
}

void FileExplorerWidget::expandNode() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeController->setExpanded(currentNodePath);
  updateDimensions();
}

void FileExplorerWidget::collapseNode() {
  auto snapshot = fileTreeController->getTreeSnapshot();
  std::string currentNodePath;
  for (auto &node : snapshot.nodes) {
    if (node.is_current) {
      currentNodePath = node.path.c_str();
      break;
    }
  }

  if (currentNodePath.empty()) {
    if (snapshot.nodes.empty()) {
      return;
    }

    fileTreeController->setCurrent(snapshot.nodes[0].path.c_str());
    return;
  }

  fileTreeController->setCollapsed(currentNodePath);
  updateDimensions();
}

int FileExplorerWidget::convertMousePositionToRow(double yPos) {
  const double lineHeight = fontMetrics.height();
  const int scrollY = verticalScrollBar()->value();

  const int targetRow = static_cast<int>((yPos + scrollY) / lineHeight);

  return targetRow;
}

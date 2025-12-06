#include "file_explorer_widget.h"
#include <neko_core.h>

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QScrollArea(parent), tree(nullptr) {
  neko_file_tree_new("");
}

FileExplorerWidget::~FileExplorerWidget() {}

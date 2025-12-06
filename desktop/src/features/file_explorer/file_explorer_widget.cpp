#include "file_explorer_widget.h"

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QScrollArea(parent), tree(nullptr) {}

FileExplorerWidget::~FileExplorerWidget() {}

#pragma once
#ifdef Q_OS_MAC
#include "types/qt_types_fwd.h"

QT_FWD(QWidget)

// NOLINTNEXTLINE
void setupMacOSTitleBar(QWidget *widget);
void disableWindowAnimation(QWidget *widget);
#endif

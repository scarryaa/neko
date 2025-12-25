#include "mac_utils.h"
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <QWidget>
#include <objc/objc.h>

// NOLINTNEXTLINE
void setupMacOSTitleBar(QWidget *widget) {
  // NOLINTNEXTLINE
  auto *view = (NSView *)widget->winId();
  NSWindow *window = [view window];

  window.titleVisibility = NSWindowTitleHidden;
  window.titlebarAppearsTransparent = YES;
  window.styleMask |= NSWindowStyleMaskMiniaturizable |
                      NSWindowStyleMaskResizable | NSWindowStyleMaskClosable |
                      NSWindowStyleMaskTitled |
                      NSWindowStyleMaskFullSizeContentView;
}

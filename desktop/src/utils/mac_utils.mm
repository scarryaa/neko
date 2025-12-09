#include "mac_utils.h"
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <objc/objc.h>

void setupMacOSTitleBar(QWidget *widget) {
  NSView *view = (NSView *)widget->winId();
  NSWindow *window = [view window];

  window.titleVisibility = NSWindowTitleHidden;
  window.titlebarAppearsTransparent = YES;
  window.styleMask |= NSWindowStyleMaskMiniaturizable |
                      NSWindowStyleMaskResizable | NSWindowStyleMaskClosable |
                      NSWindowStyleMaskTitled |
                      NSWindowStyleMaskFullSizeContentView;
}

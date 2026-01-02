#ifndef TAB_CORE_API_H
#define TAB_CORE_API_H

#include "types/ffi_types_fwd.h"
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

struct ITabCoreApi {
  virtual ~ITabCoreApi() = default;

  virtual neko::TabsSnapshot getTabsSnapshot() = 0;
  virtual std::vector<int>
  getCloseTabIds(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
                 bool closePinned) = 0;

  virtual neko::CreateDocumentTabAndViewResultFfi
  createDocumentTabAndView(const std::string &title, bool addTabToHistory,
                           bool activateView) = 0;
  uint64_t openFile(const std::string &path, bool addToHistory);
  virtual neko::MoveActiveTabResult moveTabBy(neko::Buffer buffer, int delta,
                                              bool useHistory) = 0;
  virtual bool moveTab(int fromIndex, int toIndex) = 0;
  virtual neko::PinTabResult pinTab(int tabId) = 0;
  virtual neko::PinTabResult unpinTab(int tabId) = 0;
  virtual neko::CloseManyTabsResult
  closeTabs(neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
            bool closePinned) = 0;

  virtual neko::TabSnapshotMaybe getTabSnapshot(int tabId) = 0;
  virtual void setActiveTab(int tabId) = 0;
  virtual void setTabScrollOffsets(int tabId,
                                   const neko::ScrollOffsetFfi &offsets) = 0;
};

#endif

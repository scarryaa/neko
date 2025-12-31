use crate::{ClosedTabInfo, TabId};
use std::{collections::HashMap, path::PathBuf};

#[derive(Debug, Default)]
pub struct ClosedTabStore {
    closed_tabs: HashMap<TabId, ClosedTabInfo>,
}

impl ClosedTabStore {
    // TODO(scarlet): Add session restoration persistance? E.g. allowing history navigation even if
    // history is empty by reopening past tabs
    pub fn record_closed_tabs<F>(&mut self, ids: &[TabId], mut get_tab_info: F)
    where
        F: FnMut(TabId) -> Option<ClosedTabInfo>,
    {
        for &id in ids {
            if let Some(info) = get_tab_info(id) {
                self.closed_tabs.insert(id, info);
            } else {
                // If there is no path, remove from history
                _ = self.take(id);
            }
        }
    }

    pub fn path_for(&self, id: TabId) -> Option<PathBuf> {
        self.closed_tabs.get(&id).map(|info| info.path.clone())
    }

    pub fn take(&mut self, id: TabId) -> Option<ClosedTabInfo> {
        self.closed_tabs.remove(&id)
    }

    pub fn has(&self, id: TabId) -> bool {
        self.closed_tabs.contains_key(&id)
    }
}

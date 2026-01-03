use super::{ClosedTabStore, TabHistoryManager};
use crate::{
    CloseTabOperationType, ClosedTabInfo, DocumentId, MoveActiveTabResult, MoveTabError,
    ScrollOffsets, Tab, TabError, TabId, ViewId,
};

pub enum ResolveOutcome {
    Existing(TabId),
    NeedsReopen {
        original_id: TabId,
        info: ClosedTabInfo,
    },
    Unresolvable,
}

// TODO(scarlet): Add more tests
#[derive(Debug)]
pub struct TabManager {
    tabs: Vec<Tab>,
    active_tab_id: TabId,
    next_tab_id: TabId,
    history_manager: TabHistoryManager,
    _closed_store: ClosedTabStore,
}

impl Default for TabManager {
    fn default() -> Self {
        Self::new()
    }
}

impl TabManager {
    pub fn new() -> Self {
        Self {
            tabs: Vec::new(),
            active_tab_id: TabId::new(1).expect("Tab id should not be 0"),
            next_tab_id: TabId::new(2).expect("Tab id should not be 0"),
            history_manager: TabHistoryManager::new(),
            _closed_store: ClosedTabStore::default(),
        }
    }

    // Getters
    pub fn get_history_manager(&self) -> &TabHistoryManager {
        &self.history_manager
    }

    pub fn get_history_manager_mut(&mut self) -> &mut TabHistoryManager {
        &mut self.history_manager
    }

    pub fn get_tabs(&self) -> &Vec<Tab> {
        &self.tabs
    }

    pub fn get_tabs_mut(&mut self) -> &mut Vec<Tab> {
        &mut self.tabs
    }

    pub fn get_tab(&self, id: TabId) -> Result<&Tab, TabError> {
        self.tabs
            .iter()
            .find(|tab| tab.get_id() == id)
            .ok_or(TabError::NotFound(id))
    }

    pub fn get_tab_mut(&mut self, id: TabId) -> Result<&mut Tab, TabError> {
        self.tabs
            .iter_mut()
            .find(|tab| tab.get_id() == id)
            .ok_or(TabError::NotFound(id))
    }

    pub fn get_active_tab_id(&self) -> TabId {
        self.active_tab_id
    }

    pub fn get_close_tab_ids(
        &self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: Option<TabId>,
        close_pinned: bool,
    ) -> Result<Vec<TabId>, TabError> {
        use CloseTabOperationType::*;

        let find_anchor_index = || -> Result<usize, TabError> {
            let anchor_id = anchor_tab_id.ok_or(TabError::NoIdProvided)?;
            self.tabs
                .iter()
                .position(|tab| tab.get_id() == anchor_id)
                .ok_or(TabError::NotFound(anchor_id))
        };

        let mut ids = Vec::new();

        for (idx, tab) in self.tabs.iter().enumerate() {
            let id = tab.get_id();
            let is_anchor = anchor_tab_id.map(|a| a == id).unwrap_or(false);
            let is_pinned = tab.get_is_pinned();

            // Pinned tabs handling:
            // - For a single tab close op: may close the anchor tab, even if it is pinned,
            //   if close_pinned is true && is_anchor is true
            // - For the other ops: never close pinned tabs
            if is_pinned && !(matches!(operation_type, Single) && is_anchor && close_pinned) {
                continue;
            }

            let include = match operation_type {
                Single => is_anchor,
                Others => !is_anchor,
                Left => idx < find_anchor_index()?,
                Right => idx > find_anchor_index()?,
                // Select all tabs for now, AppState will filter out modified tabs after
                All | Clean => true, // No anchor needed
            };

            if include {
                ids.push(id);
            }
        }

        Ok(ids)
    }

    fn generate_next_id(&mut self) -> TabId {
        self.next_tab_id.take_next()
    }

    pub fn find_tab_by_document(&self, document_id: DocumentId) -> Option<TabId> {
        self.tabs
            .iter()
            .find(|tab| tab.get_document_id() == document_id)
            .map(|tab| tab.get_id())
    }

    // Setters
    pub fn update_tab_view_id(&mut self, tab_id: TabId, view_id: ViewId) -> Result<(), TabError> {
        let tab = self.get_tab_mut(tab_id)?;
        tab.set_view_id(view_id);
        Ok(())
    }

    /// Creates a new tab associated with a given [`DocumentId`] and activates it, optionally
    /// adding it to history.
    pub fn add_tab_for_document(
        &mut self,
        document_id: DocumentId,
        view_id: ViewId,
        add_to_history: bool,
    ) -> TabId {
        let new_tab_id = self.generate_next_id();
        let tab = Tab::new(new_tab_id, document_id, view_id);

        self.tabs.push(tab);
        if add_to_history {
            self.activate_tab(new_tab_id);
        } else {
            self.activate_tab_no_history(new_tab_id);
        }

        new_tab_id
    }

    // TODO(scarlet): Create a unified fn for removing tabs (and adjusting history)
    pub fn close_tab(&mut self, id: TabId, history_enabled: bool) -> Result<(), TabError> {
        let index = self
            .tabs
            .iter()
            .position(|tab| tab.get_id() == id)
            .ok_or(TabError::NotFound(id))?;

        self.record_closed_tabs(&[id]);
        self.tabs.remove(index);
        self.switch_to_last_active_tab(history_enabled);

        Ok(())
    }

    pub fn close_tabs(
        &mut self,
        ids: Vec<TabId>,
        history_enabled: bool,
    ) -> Result<Vec<TabId>, TabError> {
        if ids.is_empty() {
            return Ok(ids);
        }

        self.record_closed_tabs(&ids);

        let id_set: std::collections::HashSet<TabId> = ids.iter().copied().collect();
        self.tabs.retain(|t| !id_set.contains(&t.get_id()));

        self.switch_to_last_active_tab(history_enabled);
        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    // TODO(scarlet): Break this fn down and rename
    pub fn move_active_tab_by(
        &mut self,
        delta: i64,
        use_history: bool,
        auto_reopen_closed_tabs_in_history: bool,
    ) -> MoveActiveTabResult {
        if self.tabs.is_empty() {
            // TODO(scarlet): Add a helper fn to construct the MoveActiveTabResult
            return MoveActiveTabResult {
                found_id: Some(self.active_tab_id),
                reopened_tab: false,
                scroll_offsets: None,
                cursors: None,
                selections: None,
                reopen_path: None,
            };
        }

        let history_enabled = use_history;

        // History disabled, use linear order
        if !history_enabled {
            let tab_count = self.tabs.len() as i64;
            let current_idx = self
                .tabs
                .iter()
                .position(|t| t.get_id() == self.active_tab_id)
                .unwrap_or(0) as i64;

            let next_idx = (current_idx + delta).rem_euclid(tab_count) as usize;
            let tab_id = self.tabs[next_idx].get_id();
            self.activate_tab(tab_id);

            // We don't need the cursors, selections, or scroll offsets here since the tab is already open
            return MoveActiveTabResult {
                found_id: Some(tab_id),
                reopened_tab: false,
                scroll_offsets: None,
                cursors: None,
                selections: None,
                reopen_path: None,
            };
        }

        // If there are no tabs in history, do nothing
        if self.history_manager.is_empty() || delta == 0 {
            return MoveActiveTabResult {
                found_id: Some(self.active_tab_id),
                reopened_tab: false,
                scroll_offsets: None,
                cursors: None,
                selections: None,
                reopen_path: None,
            };
        }

        // Determine starting index in the visit log
        let mut start_idx = self.history_manager.nav_start_index();
        let going_back = delta < 0;

        loop {
            if self.history_manager.is_empty() {
                self.history_manager.set_history_pos(None);
                return MoveActiveTabResult {
                    found_id: Some(self.active_tab_id),
                    reopened_tab: false,
                    scroll_offsets: None,
                    cursors: None,
                    selections: None,
                    reopen_path: None,
                };
            }

            let next_idx_opt = if delta < 0 {
                // Backwards
                self.history_manager
                    .previous_distinct_history_entry(start_idx)
            } else {
                // Forwards
                self.history_manager.next_distinct_history_entry(start_idx)
            };

            let next_idx = match next_idx_opt {
                Some(i) => i,
                None => {
                    self.history_manager.set_history_pos(Some(start_idx));
                    return MoveActiveTabResult {
                        found_id: Some(self.active_tab_id),
                        reopened_tab: false,
                        scroll_offsets: None,
                        cursors: None,
                        selections: None,
                        reopen_path: None,
                    };
                }
            };

            let original_tab_id = self.history_manager.id_at(next_idx);

            match self.resolve_history_target(original_tab_id, auto_reopen_closed_tabs_in_history) {
                ResolveOutcome::Existing(id) => {
                    self.active_tab_id = id;
                    self.history_manager.set_history_pos(Some(next_idx));
                    return MoveActiveTabResult {
                        found_id: Some(id),
                        reopened_tab: false,
                        scroll_offsets: None,
                        cursors: None,
                        selections: None,
                        reopen_path: None,
                    };
                }
                ResolveOutcome::NeedsReopen {
                    original_id: id,
                    info,
                } => {
                    self.history_manager.set_history_pos(Some(next_idx));

                    return MoveActiveTabResult {
                        found_id: Some(id),
                        reopened_tab: true,
                        scroll_offsets: Some(ScrollOffsets {
                            x: info.scroll_offsets.0,
                            y: info.scroll_offsets.1,
                        }),
                        cursors: Some(info.cursors),
                        selections: Some(info.selections),
                        reopen_path: Some(info.path.clone()),
                    };
                }
                ResolveOutcome::Unresolvable => {
                    // Drop this entry from history and keep looking
                    self.history_manager.remove_index_from_history(next_idx);

                    if self.history_manager.is_empty() {
                        self.history_manager.set_history_pos(None);
                        return MoveActiveTabResult {
                            found_id: Some(self.active_tab_id),
                            reopened_tab: false,
                            scroll_offsets: None,
                            cursors: None,
                            selections: None,
                            reopen_path: None,
                        };
                    }

                    // Adjust start_idx
                    if going_back {
                        start_idx = next_idx.saturating_sub(1);
                    } else {
                        start_idx = next_idx.min(self.history_manager.history_len() - 1);
                    }

                    continue;
                }
            }
        }
    }

    pub fn pin_tab(&mut self, id: TabId) -> Result<(), TabError> {
        let idx = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or(TabError::NotFound(id))?;

        // Bail if the tab is already pinned
        if self.tabs[idx].get_is_pinned() {
            return Ok(());
        }

        self.tabs[idx].set_is_pinned(true);

        let tab = self.tabs.remove(idx);
        let insert_idx = self
            .tabs
            .iter()
            .position(|t| !t.get_is_pinned())
            .unwrap_or(self.tabs.len());

        self.tabs.insert(insert_idx, tab);

        Ok(())
    }

    pub fn unpin_tab(&mut self, id: TabId) -> Result<(), TabError> {
        let idx = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or(TabError::NotFound(id))?;

        if !self.tabs[idx].get_is_pinned() {
            return Ok(());
        }

        self.tabs[idx].set_is_pinned(false);

        let tab = self.tabs.remove(idx);
        let insert_idx = match self.tabs.iter().rposition(|t| t.get_is_pinned()) {
            Some(last_pinned_idx) => last_pinned_idx + 1,
            None => 0,
        };

        self.tabs.insert(insert_idx, tab);

        Ok(())
    }

    pub fn set_tab_scroll_offsets(
        &mut self,
        id: TabId,
        new_offsets: (i32, i32),
    ) -> Result<(), TabError> {
        self.get_tab_mut(id).map(|tab| {
            tab.set_scroll_offsets(new_offsets);
        })
    }

    pub fn set_active_tab(&mut self, id: TabId) -> Result<(), TabError> {
        // Ensure the tab exists before activating it
        self.get_tab(id)?;
        self.activate_tab(id);
        Ok(())
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), TabError> {
        let len = self.tabs.len();

        if from >= len || to >= len {
            return Err(MoveTabError::InvalidIndex { from, to }.into());
        }

        if from == to || len <= 1 {
            return Ok(());
        }

        let tab = self.tabs.remove(from);
        self.tabs.insert(to, tab);
        Ok(())
    }

    // History ops
    fn activate_tab(&mut self, id: TabId) {
        if self.active_tab_id == id {
            return;
        }

        self.active_tab_id = id;
        self.history_manager.record_id(id);

        // Explicit activation cancels history navigation mode
        self.history_manager.set_history_pos(None);
    }

    fn activate_tab_no_history(&mut self, id: TabId) {
        self.active_tab_id = id;
    }

    fn switch_to_last_active_tab(&mut self, history_enabled: bool) {
        // No tabs left
        if self.tabs.is_empty() {
            self.active_tab_id = TabId::new(1).expect("Tab id cannot be 0");
            self.history_manager.set_history_pos(None);
            return;
        }

        if history_enabled && !self.history_manager.is_empty() {
            // Walk history backwards and pick the most recent tab that still exists
            if let Some(idx) = self
                .history_manager
                .last_matching(|id| self.tabs.iter().any(|t| t.get_id() == id))
            {
                let id = self.history_manager.id_at(idx);
                self.active_tab_id = id;
                self.history_manager.set_history_pos(Some(idx));
                return;
            }
        }

        // If history is disabled or no valid entry was found,
        // pick the last tab in the list
        // TODO(scarlet): Handle the case where there are no tabs
        self.active_tab_id = self.tabs.last().unwrap().get_id();
        self.history_manager.set_history_pos(None);
    }

    fn resolve_history_target(
        &mut self,
        _id: TabId,
        _auto_reopen_closed_tabs_in_history: bool,
    ) -> ResolveOutcome {
        ResolveOutcome::Unresolvable
        // TODO(scarlet): Adjust this for the new document architecture
        // // If the tab still exists, use it
        // if self.tabs.iter().any(|t| t.get_id() == id) {
        //     return ResolveOutcome::Existing(id);
        // }
        //
        // // Try to get the path for this history id from closed_tabs
        // let maybe_path = self.closed_store.path_for(id);
        //
        // // If there is a currently open tab with the same path (even if id is different), target that
        // if let Some(ref path) = maybe_path {
        //     if let Some(tab) = self
        //         .tabs
        //         .iter()
        //         .find(|t| t.get_file_path() == Some(&path.clone()))
        //     {
        //         let new_id = tab.get_id();
        //
        //         // Update history to point at the new id instead of the old one
        //         self.history_manager.remap_id(id, new_id);
        //         _ = self.closed_store.take(id);
        //
        //         return ResolveOutcome::Existing(new_id);
        //     }
        // }
        //
        // // If the config setting is turned off, skip
        // if !auto_reopen_closed_tabs_in_history {
        //     return ResolveOutcome::Unresolvable;
        // }
        //
        // // TODO(scarlet): Update history in the case where we go backwards a few tabs and then do a
        // // new action (e.g. tab opened)
        // //
        // // Ex.
        // // - open tabs [1, 2, 3, 4]
        // //     active_tab_history = [1, 2, 3, 4]
        // //                                       ^ history_pos = None
        // // - navigate backwards in history to tab 2
        // //     active_tab_history = [1, 2, 3, 4]
        // //                              ^ history_pos = index of 2 => 1
        // // - open a new tab 5 while history_pos points at tab 2
        // //     => should truncate [3, 4], append 5
        // //     active_tab_history = [1, 2, 5] (instead of [1, 2, 3, 4, 5])
        // //                                 ^ history_pos = index of 5 => 2
        // //                                   (should it be reset to None?)
        // // TODO(scarlet): Update history in the case where, when opening a file and then closing
        // // the tab, moving backwards in history should reopen it (currently we have to move forward
        // // in history to reopen it)
        //
        // // If there is recorded tab info, pass it along
        // let Some(info) = self.closed_store.take(id) else {
        //     return ResolveOutcome::Unresolvable;
        // };
        //
        // ResolveOutcome::NeedsReopen {
        //     original_id: id,
        //     info,
        // }
    }

    fn record_closed_tabs(&mut self, _ids: &[TabId]) {
        // TODO(scarlet): Adjust this for the new document architecture
        // self.closed_store.record_closed_tabs(ids, |id| {
        //     let path: PathBuf;
        //     let scroll_offsets: (usize, usize);
        //     let cursors: Vec<CursorEntry>;
        //     let selections: Selection;
        //
        //     if let Some(tab) = self.tabs.iter().find(|t| t.get_id() == id) {
        //         if let Some(found_path) = tab.get_file_path().map(|p| p.to_path_buf()) {
        //             let found_scroll_offsets = tab.get_scroll_offsets();
        //             scroll_offsets = (
        //                 found_scroll_offsets.0 as usize,
        //                 found_scroll_offsets.1 as usize,
        //             );
        //
        //             path = found_path;
        //
        //             let found_cursors = tab.get_editor().cursors();
        //             cursors = found_cursors;
        //
        //             let found_selections = tab.get_editor().selection();
        //             selections = found_selections;
        //         } else {
        //             return None;
        //         }
        //     } else {
        //         return None;
        //     }
        //
        //     Some(ClosedTabInfo {
        //         path,
        //         scroll_offsets: (scroll_offsets.0, scroll_offsets.1),
        //         cursors,
        //         selections,
        //     })
        // });
    }
}

#[cfg(test)]
mod test {
    use super::*;

    fn make_tab_id(id: u64) -> TabId {
        TabId::new(id).expect("Provided tab id must be non-zero")
    }

    #[test]
    fn close_tab_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new();
        let result = tm.close_tab(make_tab_id(1), false);

        assert!(result.is_err())
    }

    #[test]
    fn close_tab_returns_error_when_id_not_found() {
        let mut tm = TabManager::new();
        let result = tm.close_tab(make_tab_id(1), false);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new();
        let _ = tm.close_tab(make_tab_id(1), false);
        let result = tm.set_active_tab(make_tab_id(1));

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_id_not_found() {
        let mut tm = TabManager::new();
        let result = tm.set_active_tab(make_tab_id(1));

        assert!(result.is_err())
    }

    #[test]
    // TODO(scarlet): Fix this
    fn pin_tab_reorders_tabs_and_updates_active_index() {
        // let mut tm = TabManager::new();
        // tm.new_tab(true);
        // tm.new_tab(true);
        //
        // tm.tabs[0].set_title("A");
        // tm.tabs[1].set_title("B");
        // tm.tabs[2].set_title("C");
        //
        // let _ = tm.set_active_tab(2);
        // tm.pin_tab(2).unwrap();
        //
        // assert_eq!(
        //     tm.get_tabs()
        //         .iter()
        //         .map(|t| t.get_title())
        //         .collect::<Vec<String>>(),
        //     vec!["C", "A", "B"]
        // );
        // assert_eq!(tm.get_active_tab_id(), 2);
    }
}

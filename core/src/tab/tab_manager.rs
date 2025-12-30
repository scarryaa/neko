use super::{
    ClosedTabInfo, ClosedTabStore, MoveActiveTabResult, ScrollOffsets, Tab, TabHistoryManager,
};
use crate::{CloseTabOperationType, CursorEntry, Editor, Selection};
use std::{
    io::{Error, ErrorKind},
    path::PathBuf,
};

pub enum ResolveOutcome {
    Existing(usize),
    NeedsReopen {
        original_id: usize,
        info: ClosedTabInfo,
    },
    Unresolvable,
}

// TODO(scarlet): Add more tests
#[derive(Debug)]
pub struct TabManager {
    tabs: Vec<Tab>,
    active_tab_id: usize,
    next_tab_id: usize,
    history_manager: TabHistoryManager,
    closed_store: ClosedTabStore,
}

impl TabManager {
    pub fn new() -> Result<Self, std::io::Error> {
        let mut state = Self {
            tabs: Vec::new(),
            active_tab_id: 0,
            next_tab_id: 0,
            history_manager: TabHistoryManager::new(),
            closed_store: ClosedTabStore::default(),
        };

        state.new_tab(true);
        Ok(state)
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

    pub fn get_tab(&self, id: usize) -> Result<&Tab, Error> {
        if let Some(tab) = self.tabs.iter().find(|t| t.get_id() == id) {
            Ok(tab)
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn get_tab_mut(&mut self, id: usize) -> Result<&mut Tab, Error> {
        if let Some(tab) = self.tabs.iter_mut().find(|t| t.get_id() == id) {
            Ok(tab)
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn get_active_tab_id(&self) -> usize {
        self.active_tab_id
    }

    pub fn get_active_editor(&self) -> Option<&Editor> {
        if let Some(index) = self
            .tabs
            .iter()
            .position(|t| t.get_id() == self.active_tab_id)
        {
            self.tabs.get(index).map(|t| t.get_editor())
        } else {
            None
        }
    }

    pub fn get_active_editor_mut(&mut self) -> Option<&mut Editor> {
        if let Some(index) = self
            .tabs
            .iter()
            .position(|t| t.get_id() == self.active_tab_id)
        {
            self.tabs.get_mut(index).map(|t| t.get_editor_mut())
        } else {
            None
        }
    }

    pub fn get_close_tab_ids(
        &self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: usize,
        close_pinned: bool,
    ) -> Result<Vec<usize>, Error> {
        use CloseTabOperationType::*;

        let anchor_index = match operation_type {
            Single | Others | Left | Right => self
                .tabs
                .iter()
                .position(|t| t.get_id() == anchor_tab_id)
                .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?,
            All | Clean => 0, // No anchor needed
        };

        let mut ids = Vec::new();
        for (idx, tab) in self.tabs.iter().enumerate() {
            let id = tab.get_id();
            let is_anchor = id == anchor_tab_id;
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
                Left => idx < anchor_index,
                Right => idx > anchor_index,
                All => true,
                Clean => !tab.get_modified(),
            };

            if include {
                ids.push(id);
            }
        }

        Ok(ids)
    }

    // Setters
    pub fn new_tab(&mut self, add_to_history: bool) -> (usize, usize) {
        let id = self.get_next_tab_id();
        let tab = Tab::new(id);

        self.tabs.push(tab);
        if add_to_history {
            self.activate_tab(id);
        } else {
            self.activate_tab_no_history(id);
        }

        (id, self.tabs.len())
    }

    // TODO(scarlet): Create a unified fn for removing tabs (and adjusting history)
    pub fn close_tab(&mut self, id: usize, history_enabled: bool) -> Result<(), Error> {
        if let Some(index) = self.tabs.iter().position(|t| t.get_id() == id) {
            self.record_closed_tabs(&[id]);
            self.tabs.remove(index);
            self.switch_to_last_active_tab(history_enabled);

            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn close_tabs(
        &mut self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: usize,
        history_enabled: bool,
        close_pinned: bool,
    ) -> Result<Vec<usize>, Error> {
        let ids = self.get_close_tab_ids(operation_type, anchor_tab_id, close_pinned)?;
        if ids.is_empty() {
            return Ok(ids);
        }

        self.record_closed_tabs(&ids);

        let id_set: std::collections::HashSet<usize> = ids.iter().copied().collect();
        self.tabs.retain(|t| !id_set.contains(&t.get_id()));

        self.switch_to_last_active_tab(history_enabled);
        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    // TODO(scarlet): Break this fn down
    pub fn move_active_tab_by(
        &mut self,
        delta: i64,
        use_history: bool,
        auto_reopen_closed_tabs_in_history: bool,
    ) -> MoveActiveTabResult {
        if self.tabs.is_empty() {
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

    pub fn pin_tab(&mut self, id: usize) -> Result<(), Error> {
        let idx = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

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

    pub fn unpin_tab(&mut self, id: usize) -> Result<(), Error> {
        let idx = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

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
        id: usize,
        new_offsets: (i32, i32),
    ) -> Result<(), Error> {
        if let Some(tab) = self.tabs.iter_mut().find(|t| t.get_id() == id) {
            tab.set_scroll_offsets(new_offsets);
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn set_active_tab(&mut self, id: usize) -> Result<(), Error> {
        if self.tabs.iter().any(|t| t.get_id() == id) {
            self.activate_tab(id);
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), Error> {
        let len = self.tabs.len();
        if from >= len || to >= len {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "Invalid tab index",
            ));
        }

        if from == to || len <= 1 {
            return Ok(());
        }

        let tab = self.tabs.remove(from);
        self.tabs.insert(to, tab);

        Ok(())
    }

    fn get_next_tab_id(&mut self) -> usize {
        let id = self.next_tab_id;
        self.next_tab_id += 1;

        id
    }

    // History ops
    fn activate_tab(&mut self, id: usize) {
        if self.active_tab_id == id {
            return;
        }

        self.active_tab_id = id;
        self.history_manager.record_id(id);

        // Explicit activation cancels history navigation mode
        self.history_manager.set_history_pos(None);
    }

    fn activate_tab_no_history(&mut self, id: usize) {
        self.active_tab_id = id;
    }

    fn switch_to_last_active_tab(&mut self, history_enabled: bool) {
        // No tabs left
        if self.tabs.is_empty() {
            self.active_tab_id = 0;
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
        self.active_tab_id = self.tabs.last().unwrap().get_id();
        self.history_manager.set_history_pos(None);
    }

    fn resolve_history_target(
        &mut self,
        id: usize,
        auto_reopen_closed_tabs_in_history: bool,
    ) -> ResolveOutcome {
        // If the tab still exists, use it
        if self.tabs.iter().any(|t| t.get_id() == id) {
            return ResolveOutcome::Existing(id);
        }

        // Try to get the path for this history id from closed_tabs
        let maybe_path = self.closed_store.path_for(id);

        // If there is a currently open tab with the same path (even if id is different), target that
        if let Some(ref path) = maybe_path {
            if let Some(tab) = self
                .tabs
                .iter()
                .find(|t| t.get_file_path() == Some(&path.clone()))
            {
                let new_id = tab.get_id();

                // Update history to point at the new id instead of the old one
                self.history_manager.remap_id(id, new_id);
                _ = self.closed_store.take(id);

                return ResolveOutcome::Existing(new_id);
            }
        }

        // If the config setting is turned off, skip
        if !auto_reopen_closed_tabs_in_history {
            return ResolveOutcome::Unresolvable;
        }

        // TODO(scarlet): Update history in the case where we go backwards a few tabs and then do a
        // new action (e.g. tab opened)
        //
        // Ex.
        // - open tabs [1, 2, 3, 4]
        //     active_tab_history = [1, 2, 3, 4]
        //                                       ^ history_pos = None
        // - navigate backwards in history to tab 2
        //     active_tab_history = [1, 2, 3, 4]
        //                              ^ history_pos = index of 2 => 1
        // - open a new tab 5 while history_pos points at tab 2
        //     => should truncate [3, 4], append 5
        //     active_tab_history = [1, 2, 5] (instead of [1, 2, 3, 4, 5])
        //                                 ^ history_pos = index of 5 => 2
        //                                   (should it be reset to None?)
        // TODO(scarlet): Update history in the case where, when opening a file and then closing
        // the tab, moving backwards in history should reopen it (currently we have to move forward
        // in history to reopen it)

        // If there is recorded tab info, pass it along
        let Some(info) = self.closed_store.take(id) else {
            return ResolveOutcome::Unresolvable;
        };

        ResolveOutcome::NeedsReopen {
            original_id: id,
            info,
        }
    }

    fn record_closed_tabs(&mut self, ids: &[usize]) {
        self.closed_store.record_closed_tabs(ids, |id| {
            let path: PathBuf;
            let scroll_offsets: (usize, usize);
            let cursors: Vec<CursorEntry>;
            let selections: Selection;

            if let Some(tab) = self.tabs.iter().find(|t| t.get_id() == id) {
                if let Some(found_path) = tab.get_file_path().map(|p| p.to_path_buf()) {
                    let found_scroll_offsets = tab.get_scroll_offsets();
                    scroll_offsets = (
                        found_scroll_offsets.0 as usize,
                        found_scroll_offsets.1 as usize,
                    );

                    path = found_path;

                    let found_cursors = tab.get_editor().cursors();
                    cursors = found_cursors;

                    let found_selections = tab.get_editor().selection();
                    selections = found_selections;
                } else {
                    return None;
                }
            } else {
                return None;
            }

            Some(ClosedTabInfo {
                path,
                scroll_offsets: (scroll_offsets.0, scroll_offsets.1),
                cursors,
                selections,
            })
        });
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn close_tab_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new().unwrap();
        let result = tm.close_tab(1, false);

        assert!(result.is_err())
    }

    #[test]
    fn close_tab_returns_error_when_id_not_found() {
        let mut tm = TabManager::new().unwrap();
        let result = tm.close_tab(1, false);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new().unwrap();
        let _ = tm.close_tab(0, false);
        let result = tm.set_active_tab(0);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_id_not_found() {
        let mut tm = TabManager::new().unwrap();
        let result = tm.set_active_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn pin_tab_reorders_tabs_and_updates_active_index() {
        let mut tm = TabManager::new().unwrap();
        tm.new_tab(true);
        tm.new_tab(true);

        tm.tabs[0].set_title("A");
        tm.tabs[1].set_title("B");
        tm.tabs[2].set_title("C");

        let _ = tm.set_active_tab(2);
        tm.pin_tab(2).unwrap();

        assert_eq!(
            tm.get_tabs()
                .iter()
                .map(|t| t.get_title())
                .collect::<Vec<String>>(),
            vec!["C", "A", "B"]
        );
        assert_eq!(tm.get_active_tab_id(), 2);
    }
}

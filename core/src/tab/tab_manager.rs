use super::{
    ClosedTabInfo, ClosedTabStore, MoveActiveTabResult, ScrollOffsets, Tab,
    history::TabHistoryManager,
};
use crate::{Editor, Selection, text::cursor_manager::CursorEntry};
use std::{
    io::{Error, ErrorKind},
    path::{Path, PathBuf},
};

enum ResolveOutcome {
    Existing(usize),
    Reopened {
        id: usize,
        scroll_offsets: (usize, usize),
        cursors: Vec<CursorEntry>,
        selections: Selection,
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
    pub fn get_tabs(&self) -> &Vec<Tab> {
        &self.tabs
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

    pub fn get_close_other_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        if !self.tabs.iter().any(|t| t.get_id() == id) {
            return Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ));
        }

        Ok(self
            .tabs
            .iter()
            .filter(|t| t.get_id() != id && !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_all_tab_ids(&self) -> Result<Vec<usize>, Error> {
        Ok(self
            .tabs
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_clean_tab_ids(&self) -> Result<Vec<usize>, Error> {
        Ok(self
            .tabs
            .iter()
            .filter(|t| !t.get_is_pinned() && !t.get_modified())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_left_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        Ok(self.tabs[..index]
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_right_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        Ok(self.tabs[index + 1..]
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
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

    // TODO(scarlet): Reduce duplication in 'close_*' methods
    pub fn close_other_tabs(
        &mut self,
        id: usize,
        history_enabled: bool,
    ) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_other_tab_ids(id)?;

        let exists = self.tabs.iter().any(|t| t.get_id() == id);
        if !exists {
            return Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ));
        }

        self.record_closed_tabs(&ids);

        self.tabs.retain(|t| t.get_is_pinned() || t.get_id() == id);
        self.switch_to_last_active_tab(history_enabled);

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_left_tabs(
        &mut self,
        id: usize,
        history_enabled: bool,
    ) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_left_tab_ids(id)?;

        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        self.record_closed_tabs(&ids);

        let mut i = 0usize;
        self.tabs.retain(|t| {
            let keep = i >= index || t.get_is_pinned();
            i += 1;
            keep
        });
        self.switch_to_last_active_tab(history_enabled);

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_right_tabs(
        &mut self,
        id: usize,
        history_enabled: bool,
    ) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_right_tab_ids(id)?;

        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        self.record_closed_tabs(&ids);

        let mut i = 0usize;
        self.tabs.retain(|t| {
            let keep = i <= index || t.get_is_pinned();
            i += 1;
            keep
        });
        self.switch_to_last_active_tab(history_enabled);

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_all_tabs(&mut self, history_enabled: bool) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_all_tab_ids()?;

        self.record_closed_tabs(&ids);

        // Remove all non-pinned tabs, keeping order of pinned ones
        self.tabs.retain(|t| t.get_is_pinned());
        self.switch_to_last_active_tab(history_enabled);

        Ok(ids)
    }

    pub fn close_clean_tabs(&mut self, history_enabled: bool) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_clean_tab_ids()?;

        self.record_closed_tabs(&ids);

        // Remove all clean non-pinned tabs, keeping order of pinned ones
        self.tabs.retain(|t| t.get_is_pinned() || t.get_modified());
        self.switch_to_last_active_tab(history_enabled);

        Ok(ids)
    }

    // Returns the id and a flag indicating whether a closed tab was reopened
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
                    };
                }
                ResolveOutcome::Reopened {
                    id,
                    scroll_offsets,
                    cursors,
                    selections,
                } => {
                    self.active_tab_id = id;
                    self.history_manager.set_history_pos(Some(next_idx));
                    return MoveActiveTabResult {
                        found_id: Some(id),
                        reopened_tab: true,
                        scroll_offsets: Some(ScrollOffsets {
                            x: scroll_offsets.0,
                            y: scroll_offsets.1,
                        }),
                        cursors: Some(cursors),
                        selections: Some(selections),
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

    pub fn open_file(&mut self, path: &str) -> Result<usize, std::io::Error> {
        // Check if file is already open
        if self
            .tabs
            .iter()
            .any(|t| t.get_file_path().as_ref().and_then(|p| p.to_str()) == Some(path))
        {
            return Err(Error::new(
                ErrorKind::AlreadyExists,
                "File with given path already open",
            ));
        }

        let content = std::fs::read_to_string(path)?;

        if let Some(t) = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
        {
            t.get_editor_mut().load_file(&content);
            t.set_original_content(content);
            t.set_file_path(Some(path.into()));
            t.set_title(
                Path::new(path)
                    .file_name()
                    .and_then(|n| n.to_str())
                    .unwrap_or(path),
            );
        }

        Ok(self.active_tab_id)
    }

    // TODO: Extract duplicated logic in the save_active_tab_* methods
    pub fn save_active_tab(&mut self) -> Result<(), std::io::Error> {
        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let maybe_file_path = tab.get_file_path();
        let path = maybe_file_path
            .as_ref()
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No current file"))?;

        let content = tab.get_editor().buffer().get_text();
        std::fs::write(path, &content)?;

        tab.set_original_content(content.clone());

        Ok(())
    }

    pub fn save_active_tab_and_set_path(&mut self, path: &str) -> Result<(), std::io::Error> {
        if path.is_empty() {
            return Err(Error::new(ErrorKind::InvalidInput, "Path cannot be empty"));
        }

        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let content = tab.get_editor().buffer().get_text();
        tab.set_original_content(content.clone());
        std::fs::write(path, content)?;
        tab.set_file_path(Some(path.to_string()));

        if let Some(filename) = PathBuf::from(path).file_name() {
            tab.set_title(&filename.to_string_lossy());
        }

        Ok(())
    }

    pub fn save_tab_with_id(&mut self, id: usize) -> Result<(), Error> {
        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        let path: PathBuf = tab
            .get_file_path()
            .ok_or_else(|| Error::new(ErrorKind::InvalidInput, "Tab path is missing"))?
            .to_path_buf();

        let content = tab.get_editor().buffer().get_text();

        std::fs::write(&path, &content)?;

        tab.set_original_content(content);

        if let Some(filename) = path.file_name() {
            tab.set_title(&filename.to_string_lossy());
        }

        Ok(())
    }

    pub fn save_tab_with_id_and_set_path(&mut self, id: usize, path: &str) -> Result<(), Error> {
        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        let content = tab.get_editor().buffer().get_text();
        tab.set_original_content(content.clone());
        std::fs::write(path, content)?;
        tab.set_file_path(Some(path.to_string()));

        if let Some(filename) = PathBuf::from(path).file_name() {
            tab.set_title(&filename.to_string_lossy());
        }

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

        // If there is recorded tab info, try to reopen it
        let Some(info) = self.closed_store.take(id) else {
            return ResolveOutcome::Unresolvable;
        };

        let (new_id, _) = self.new_tab(false);
        if self.open_file(&info.path.to_string_lossy()).is_err() {
            return ResolveOutcome::Unresolvable;
        }

        // Update the tab id in history
        self.history_manager.remap_id(id, new_id);

        // Grab the scroll offsets
        ResolveOutcome::Reopened {
            id: new_id,
            scroll_offsets: info.scroll_offsets,
            cursors: info.cursors,
            selections: info.selections,
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
    fn open_file_returns_error_when_file_is_already_open() {
        let mut tm = TabManager::new().unwrap();
        tm.new_tab(true);
        tm.tabs[1].set_file_path(Some("test".to_string()));

        let result = tm.open_file("test");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new().unwrap();
        let _ = tm.close_tab(0, false);
        let result = tm.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_there_is_no_current_file() {
        let mut tm = TabManager::new().unwrap();
        let result = tm.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_provided_path_is_empty() {
        let mut tm = TabManager::new().unwrap();
        let result = tm.save_active_tab_and_set_path("");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_tabs_are_empty() {
        let mut tm = TabManager::new().unwrap();
        let _ = tm.close_tab(0, false);
        let result = tm.save_active_tab_and_set_path("");

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

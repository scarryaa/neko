use super::Tab;
use crate::{Editor, FileTree, config::ConfigManager};
use std::{
    collections::HashMap,
    io::{Error, ErrorKind},
    path::{Path, PathBuf},
};

#[derive(Debug)]
struct ClosedTabInfo {
    path: PathBuf,
    // title, scroll offsets, cursor pos, etc?
}

enum ResolveOutcome {
    Existing(usize),
    Reopened(usize),
    Unresolvable,
}

// TODO(scarlet): Create a separate TabManager?
#[derive(Debug)]
pub struct AppState {
    tabs: Vec<Tab>,
    config_manager: *const ConfigManager,
    active_tab_id: usize,
    file_tree: FileTree,
    next_tab_id: usize,
    /// Tracks last activated tabs by id
    active_tab_history: Vec<usize>,
    // Index into active_tab_history when navigating
    history_pos: Option<usize>,
    closed_tabs: HashMap<usize, ClosedTabInfo>,
}

impl AppState {
    pub fn new(
        config_manager: &ConfigManager,
        root_path: Option<&str>,
    ) -> Result<Self, std::io::Error> {
        let mut state = Self {
            tabs: Vec::new(),
            config_manager,
            active_tab_id: 0,
            file_tree: FileTree::new(root_path)?,
            next_tab_id: 0,
            active_tab_history: vec![0],
            history_pos: Some(0),
            closed_tabs: HashMap::new(),
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

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
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
    pub fn close_tab(&mut self, id: usize) -> Result<(), Error> {
        if let Some(index) = self.tabs.iter().position(|t| t.get_id() == id) {
            self.record_closed_tabs(&[id]);
            self.tabs.remove(index);
            self.switch_to_last_active_tab();

            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    // TODO(scarlet): Reduce duplication in 'close_*' methods
    pub fn close_other_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
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
        self.switch_to_last_active_tab();

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_left_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
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
        self.switch_to_last_active_tab();

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_right_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
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
        self.switch_to_last_active_tab();

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(ids)
    }

    pub fn close_all_tabs(&mut self) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_all_tab_ids()?;

        self.record_closed_tabs(&ids);

        // Remove all non-pinned tabs, keeping order of pinned ones
        self.tabs.retain(|t| t.get_is_pinned());
        self.switch_to_last_active_tab();

        Ok(ids)
    }

    pub fn close_clean_tabs(&mut self) -> Result<Vec<usize>, Error> {
        // Get the IDs that will be closed
        let ids = self.get_close_clean_tab_ids()?;

        self.record_closed_tabs(&ids);

        // Remove all clean non-pinned tabs, keeping order of pinned ones
        self.tabs.retain(|t| t.get_is_pinned() || t.get_modified());
        self.switch_to_last_active_tab();

        Ok(ids)
    }

    // Returns the id and a flag indicating whether a closed tab was reopened
    pub fn move_active_tab_by(&mut self, delta: i64, use_history: bool) -> (usize, bool) {
        if self.tabs.is_empty() {
            return (self.active_tab_id, false);
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
            return (tab_id, false);
        }

        // If there are no tabs in history, do nothing
        if self.active_tab_history.is_empty() || delta == 0 {
            return (self.active_tab_id, false);
        }

        // Determine starting index in the visit log
        let mut start_idx = match self.history_pos {
            None => self.active_tab_history.len() - 1,
            Some(i) => i,
        };

        let going_back = delta < 0;

        loop {
            if self.active_tab_history.is_empty() {
                self.history_pos = None;
                return (self.active_tab_id, false);
            }

            let next_idx_opt = if delta < 0 {
                // Backwards
                self.previous_distinct_history_entry(start_idx)
            } else {
                // Forwards
                self.next_distinct_history_entry(start_idx)
            };

            let next_idx = match next_idx_opt {
                Some(i) => i,
                None => {
                    self.history_pos = Some(start_idx);
                    return (self.active_tab_id, false);
                }
            };

            let original_tab_id = self.active_tab_history[next_idx];

            match self.resolve_history_target(original_tab_id) {
                ResolveOutcome::Existing(id) => {
                    self.active_tab_id = id;
                    self.history_pos = Some(next_idx);
                    return (id, false);
                }
                ResolveOutcome::Reopened(id) => {
                    self.active_tab_id = id;
                    self.history_pos = Some(next_idx);
                    return (id, true);
                }
                ResolveOutcome::Unresolvable => {
                    // Drop this entry from history and keep looking
                    self.active_tab_history.remove(next_idx);

                    if self.active_tab_history.is_empty() {
                        self.history_pos = None;
                        return (self.active_tab_id, false);
                    }

                    // Adjust start_idx
                    if going_back {
                        start_idx = next_idx.saturating_sub(1);
                    } else {
                        start_idx = next_idx.min(self.active_tab_history.len() - 1);
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

    pub fn reveal_in_file_tree(&mut self, path: &str) {
        if self.file_tree.root_path.as_os_str().is_empty() {
            return;
        }

        // Expand ancestors so the file becomes visible
        self.file_tree.set_expanded(path);

        // Mark it as current
        self.file_tree.set_current(path);
    }

    // Utility
    fn config_manager(&self) -> &ConfigManager {
        // SAFETY: C++ must ensure ConfigManager outlives AppState
        assert!(
            !self.config_manager.is_null(),
            "config_manager pointer is null"
        );
        unsafe { &*self.config_manager }
    }

    fn get_next_tab_id(&mut self) -> usize {
        let id = self.next_tab_id;
        self.next_tab_id += 1;

        id
    }

    // History ops
    fn previous_distinct_history_entry(&self, start: usize) -> Option<usize> {
        if start == 0 {
            return None;
        }
        let current = self.active_tab_history[start];
        (0..start)
            .rev()
            .find(|&i| self.active_tab_history[i] != current)
    }

    fn next_distinct_history_entry(&self, start: usize) -> Option<usize> {
        let len = self.active_tab_history.len();
        if start + 1 >= len {
            return None;
        }
        let current = self.active_tab_history[start];
        (start + 1..len).find(|&i| self.active_tab_history[i] != current)
    }

    fn remove_tabs_from_history(&mut self, ids: &Vec<usize>) {
        for id in ids {
            self.active_tab_history.retain(|&i| i != *id);
        }
    }

    fn activate_tab(&mut self, id: usize) {
        if self.active_tab_id == id {
            return;
        }

        self.active_tab_id = id;
        if self.active_tab_history.last().copied() != Some(id) {
            self.active_tab_history.push(id);
        }

        // Explicit activation cancels history navigation mode
        self.history_pos = None;
    }

    fn activate_tab_no_history(&mut self, id: usize) {
        self.active_tab_id = id;
    }

    fn switch_to_last_active_tab(&mut self) {
        // No tabs left
        if self.tabs.is_empty() {
            self.active_tab_id = 0;
            self.history_pos = None;
            return;
        }

        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;

        if history_enabled && !self.active_tab_history.is_empty() {
            // Walk history backwards and pick the most recent tab that still exists
            if let Some(idx) = self
                .active_tab_history
                .iter()
                .rposition(|id| self.tabs.iter().any(|t| t.get_id() == *id))
            {
                let id = self.active_tab_history[idx];
                self.active_tab_id = id;
                self.history_pos = Some(idx);
                return;
            }
        }

        // If history is disabled or no valid entry was found,
        // pick the last tab in the list
        self.active_tab_id = self.tabs.last().unwrap().get_id();
        self.history_pos = None;
    }

    fn resolve_history_target(&mut self, id: usize) -> ResolveOutcome {
        // If the tab still exists, use it
        if self.tabs.iter().any(|t| t.get_id() == id) {
            return ResolveOutcome::Existing(id);
        }

        // Try to get the path for this history id from closed_tabs
        let maybe_path = self.closed_tabs.get(&id).map(|info| info.path.clone());

        // If there is a currently open tab with the same path (even if id is different), target that
        if let Some(ref path) = maybe_path {
            if let Some(tab) = self.tabs.iter().find(|t| t.get_file_path() == Some(path)) {
                let new_id = tab.get_id();

                // Update history to point at the new id instead of the old one
                for history_id in &mut self.active_tab_history {
                    if *history_id == id {
                        *history_id = new_id;
                    }
                }
                self.closed_tabs.remove(&id);

                return ResolveOutcome::Existing(new_id);
            }
        }

        // If the config setting is turned off, skip
        let cfg = self.config_manager().get_snapshot();
        if !cfg.editor.auto_reopen_closed_tabs_in_history {
            return ResolveOutcome::Unresolvable;
        }

        // If there is recorded tab info, try to reopen it
        let Some(info) = self.closed_tabs.remove(&id) else {
            return ResolveOutcome::Unresolvable;
        };

        let (new_id, _) = self.new_tab(false);
        if self.open_file(&info.path.to_string_lossy()).is_err() {
            return ResolveOutcome::Unresolvable;
        }

        // Update the tab id
        for history_id in &mut self.active_tab_history {
            if *history_id == id {
                *history_id = new_id;
            }
        }

        ResolveOutcome::Reopened(new_id)
    }

    // Closed tab history
    // TODO(scarlet): Add session restoration persistance? E.g. allowing history navigation even if
    // history is empty by reopening past tabs
    fn record_closed_tabs(&mut self, ids: &[usize]) {
        for id in ids {
            if let Some(tab) = self.tabs.iter().find(|t| t.get_id() == *id) {
                if let Some(path) = tab.get_file_path() {
                    self.closed_tabs.insert(
                        *id,
                        ClosedTabInfo {
                            path: path.to_path_buf(),
                        },
                    );
                } else {
                    // If there is no path, remove from history
                    self.remove_tabs_from_history(&vec![*id]);
                }
            }
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn close_tab_returns_error_when_tabs_are_empty() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let result = a.close_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn close_tab_returns_error_when_id_not_found() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let result = a.close_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_tabs_are_empty() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let _ = a.close_tab(0);
        let result = a.set_active_tab(0);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_id_not_found() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let result = a.set_active_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn open_file_returns_error_when_file_is_already_open() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        a.new_tab(true);
        a.tabs[1].set_file_path(Some("test".to_string()));

        let result = a.open_file("test");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_tabs_are_empty() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let _ = a.close_tab(0);
        let result = a.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_there_is_no_current_file() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let result = a.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_provided_path_is_empty() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let result = a.save_active_tab_and_set_path("");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_tabs_are_empty() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        let _ = a.close_tab(0);
        let result = a.save_active_tab_and_set_path("");

        assert!(result.is_err())
    }

    #[test]
    fn pin_tab_reorders_tabs_and_updates_active_index() {
        let c = ConfigManager::default();
        let mut a = AppState::new(&c, None).unwrap();
        a.new_tab(true);
        a.new_tab(true);

        a.tabs[0].set_title("A");
        a.tabs[1].set_title("B");
        a.tabs[2].set_title("C");

        let _ = a.set_active_tab(2);
        a.pin_tab(2).unwrap();

        assert_eq!(
            a.get_tabs()
                .iter()
                .map(|t| t.get_title())
                .collect::<Vec<String>>(),
            vec!["C", "A", "B"]
        );
        assert_eq!(a.get_active_tab_id(), 2);
    }
}

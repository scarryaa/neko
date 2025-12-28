use std::{collections::HashMap, path::PathBuf};

#[derive(Debug)]
pub struct ClosedTabInfo {
    pub path: PathBuf,
    // title, scroll offsets, cursor pos, etc?
}

// TODO(scarlet): Add tests
#[derive(Debug, Default)]
pub struct TabHistoryManager {
    /// Tracks last activated tabs by id
    active_tab_history: Vec<usize>,
    // Index into active_tab_history when navigating
    history_pos: Option<usize>,
    closed_tabs: HashMap<usize, ClosedTabInfo>,
}

impl TabHistoryManager {
    pub fn new() -> Self {
        Self {
            active_tab_history: vec![0],
            history_pos: Some(0),
            closed_tabs: HashMap::new(),
        }
    }

    pub fn last_matching<F>(&self, predicate: F) -> Option<usize>
    where
        F: Fn(usize) -> bool,
    {
        self.active_tab_history
            .iter()
            .rposition(|&id| predicate(id))
    }

    pub fn id_at(&self, idx: usize) -> usize {
        self.active_tab_history[idx]
    }

    pub fn nav_start_index(&self) -> usize {
        // If we have a current position, use it; otherwise start at the last entry
        // NOTE: Unwrapping should be safe here since we return a fallback value
        self.history_pos
            .or_else(|| Some(self.active_tab_history.len() - 1))
            .unwrap()
    }

    pub fn history_len(&self) -> usize {
        self.active_tab_history.len()
    }

    pub fn is_empty(&self) -> bool {
        self.active_tab_history.is_empty()
    }

    pub fn closed_path_for(&self, id: usize) -> Option<PathBuf> {
        self.closed_tabs.get(&id).map(|info| info.path.clone())
    }

    pub fn take_closed_info(&mut self, id: usize) -> Option<ClosedTabInfo> {
        self.closed_tabs.remove(&id)
    }

    pub fn remap_id(&mut self, old_id: usize, new_id: usize) {
        for history_id in &mut self.active_tab_history {
            if *history_id == old_id {
                *history_id = new_id;
            }
        }
        self.closed_tabs.remove(&old_id);
    }

    pub fn previous_distinct_history_entry(&self, start: usize) -> Option<usize> {
        if start == 0 {
            return None;
        }

        let current = self.active_tab_history[start];
        (0..start)
            .rev()
            .find(|&i| self.active_tab_history[i] != current)
    }

    pub fn next_distinct_history_entry(&self, start: usize) -> Option<usize> {
        let len = self.active_tab_history.len();
        if start + 1 >= len {
            return None;
        }

        let current = self.active_tab_history[start];
        (start + 1..len).find(|&i| self.active_tab_history[i] != current)
    }

    pub fn remove_id_from_history(&mut self, id: usize) {
        self.active_tab_history.retain(|&h| h != id);
        self.closed_tabs.remove(&id);
    }

    pub fn remove_index_from_history(&mut self, idx: usize) {
        self.active_tab_history.remove(idx);
    }

    pub fn record_id(&mut self, id: usize) {
        if self.active_tab_history.last().copied() != Some(id) {
            self.active_tab_history.push(id);
        }
    }

    pub fn set_history_pos(&mut self, pos: Option<usize>) {
        self.history_pos = pos;
    }

    // Closed tab history
    // TODO(scarlet): Add session restoration persistance? E.g. allowing history navigation even if
    // history is empty by reopening past tabs
    pub fn record_closed_tabs<F>(&mut self, ids: &[usize], mut get_path: F)
    where
        F: FnMut(usize) -> Option<PathBuf>,
    {
        for &id in ids {
            if let Some(path) = get_path(id) {
                self.closed_tabs.insert(id, ClosedTabInfo { path });
            } else {
                // If there is no path, remove from history
                self.remove_id_from_history(id);
            }
        }
    }
}

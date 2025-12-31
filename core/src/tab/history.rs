use crate::TabId;

// TODO(scarlet): Add tests
#[derive(Debug, Default)]
pub struct TabHistoryManager {
    /// Tracks last activated tabs by id
    active_tab_history: Vec<TabId>,
    /// Index into active_tab_history when navigating
    history_pos: Option<usize>,
}

impl TabHistoryManager {
    pub fn new() -> Self {
        Self {
            active_tab_history: vec![TabId::new(1).expect("Tab id cannot be 0")],
            history_pos: Some(0),
        }
    }

    pub fn last_matching<F>(&self, predicate: F) -> Option<usize>
    where
        F: Fn(TabId) -> bool,
    {
        self.active_tab_history
            .iter()
            .rposition(|&id| predicate(id))
    }

    pub fn id_at(&self, idx: usize) -> TabId {
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

    pub fn remap_id(&mut self, old_id: TabId, new_id: TabId) {
        for history_id in &mut self.active_tab_history {
            if *history_id == old_id {
                *history_id = new_id;
            }
        }
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

    pub fn remove_id_from_history(&mut self, id: TabId) {
        self.active_tab_history.retain(|&h| h != id);
    }

    pub fn remove_index_from_history(&mut self, idx: usize) {
        self.active_tab_history.remove(idx);
    }

    pub fn record_id(&mut self, id: TabId) {
        if self.active_tab_history.last().copied() != Some(id) {
            self.active_tab_history.push(id);
        }
    }

    pub fn set_history_pos(&mut self, pos: Option<usize>) {
        self.history_pos = pos;
    }
}

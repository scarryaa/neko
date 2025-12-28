use super::{AddCursorDirection, Cursor, CursorEntry};
use crate::Buffer;

// TODO(scarlet): Add tests
#[derive(Default, Debug)]
pub struct CursorManager {
    next_cursor_id: u64,
    next_group_id: u64,
    pub(crate) cursors: Vec<CursorEntry>,
    pub(crate) active_cursor_index: usize,
}

impl CursorManager {
    pub fn new() -> Self {
        let cursor = CursorEntry::individual(0, &Cursor::new());
        let next_cursor_id = 1;

        Self {
            next_cursor_id,
            next_group_id: 0,
            cursors: vec![cursor.clone()],
            active_cursor_index: 0,
        }
    }

    pub fn get_last_added_cursor(&self) -> Cursor {
        self.cursors
            .clone()
            .into_iter()
            .max_by_key(|c| c.id)
            .map(|c| c.cursor)
            .unwrap_or(Cursor::default())
    }

    pub fn set_cursors(&mut self, cursors: Vec<CursorEntry>) {
        if cursors.is_empty() {
            let fallback = CursorEntry::individual(self.new_cursor_id(), &Cursor::new());
            self.cursors = vec![fallback];
            self.active_cursor_index = 0;
            return;
        }

        self.cursors = cursors;
        self.active_cursor_index = self
            .active_cursor_index
            .min(self.cursors.len().saturating_sub(1));
    }

    pub fn clear_cursors(&mut self) {
        let active_cursor = self.cursors[self.active_cursor_index].clone();
        self.cursors.clear();
        self.cursors.push(active_cursor);
        self.active_cursor_index = 0;
    }

    pub fn clear_and_set_cursors(&mut self, cursor: CursorEntry) {
        self.cursors.clear();
        self.cursors.push(cursor);
        self.active_cursor_index = 0;
    }

    pub fn cursor_exists_at(&self, row: usize, col: usize) -> bool {
        self.cursors
            .iter()
            .any(|c| c.cursor.row == row && c.cursor.column == col)
    }

    pub fn cursor_exists_at_row(&self, row: usize) -> bool {
        self.cursors.iter().any(|c| c.cursor.row == row)
    }

    pub fn set_active_cursor_index(&mut self, i: usize) {
        self.active_cursor_index = i;
    }

    pub fn new_cursor_id(&mut self) -> u64 {
        let id = self.next_cursor_id;
        self.next_cursor_id += 1;
        id
    }

    fn group_ids(&self) -> Vec<u64> {
        let mut gs: Vec<u64> = self.cursors.iter().filter_map(|e| e.column_group).collect();
        gs.sort_unstable();
        gs.dedup();
        gs
    }

    fn ensure_all_in_groups(&mut self) {
        let mut next = self.next_group_id;

        for i in 0..self.cursors.len() {
            if self.cursors[i].column_group.is_none() {
                self.cursors[i].column_group = Some(next);
                next += 1;
            }
        }

        self.next_group_id = next;
    }

    fn group_has_cursor_at_row(&self, group: u64, row: usize) -> bool {
        self.cursors
            .iter()
            .any(|e| e.column_group == Some(group) && e.cursor.row == row)
    }

    fn group_main(&self, group: u64) -> Option<&CursorEntry> {
        self.cursors
            .iter()
            .filter(|e| e.column_group == Some(group))
            .min_by_key(|e| e.id)
    }

    fn group_contiguous_between(&self, group: u64, a: usize, b: usize) -> bool {
        let (start, end) = if a <= b { (a, b) } else { (b, a) };
        for r in start..=end {
            if !self.group_has_cursor_at_row(group, r) {
                return false;
            }
        }
        true
    }

    fn remove_one_in_group_at_row(&mut self, group: u64, row: usize) -> bool {
        if let Some(idx) = self
            .cursors
            .iter()
            .position(|e| e.column_group == Some(group) && e.cursor.row == row)
        {
            self.cursors.remove(idx);
            return true;
        }
        false
    }

    pub fn remove_cursor(&mut self, row: usize, col: usize) {
        if self.cursors.len() == 1 {
            return;
        }

        let index = self
            .cursors
            .iter()
            .position(|c| c.cursor.row == row && c.cursor.column == col);

        if let Some(found_index) = index {
            self.cursors.remove(found_index);

            if found_index == self.active_cursor_index
                || self.active_cursor_index >= self.cursors.len()
            {
                self.active_cursor_index = self.cursors.len() - 1;
            }
        }
    }

    pub fn sort_and_dedup_cursors(&mut self) {
        let active_id = self.cursors.get(self.active_cursor_index).map(|c| c.id);

        self.cursors.sort_by(|a, b| {
            (a.cursor.row, a.cursor.column)
                .cmp(&(b.cursor.row, b.cursor.column))
                // independent first
                .then_with(|| a.column_group.is_some().cmp(&b.column_group.is_some()))
                // older first
                .then_with(|| a.id.cmp(&b.id))
        });

        self.cursors
            .dedup_by(|a, b| a.cursor.row == b.cursor.row && a.cursor.column == b.cursor.column);

        self.active_cursor_index = active_id
            .and_then(|id| self.cursors.iter().position(|c| c.id == id))
            .unwrap_or(0);
    }

    pub fn cursor_byte_idxs(&self, buffer: &Buffer) -> Vec<usize> {
        self.cursors
            .iter()
            .map(|c| c.cursor.get_idx(buffer))
            .collect()
    }

    pub fn add_cursor(&mut self, direction: AddCursorDirection, buffer: &Buffer) {
        match direction {
            AddCursorDirection::Above => self.add_cursor_above(buffer),
            AddCursorDirection::Below => self.add_cursor_below(buffer),
            AddCursorDirection::At { row, col } => self.add_cursor_at(buffer, row, col),
        }
    }

    fn add_cursor_above(&mut self, buffer: &Buffer) {
        self.ensure_all_in_groups();
        let groups = self.group_ids();

        for g in groups {
            let Some(main) = self.group_main(g) else {
                continue;
            };
            let main_row = main.cursor.row;

            // find bottommost cursor in this group
            let max_row = self
                .cursors
                .iter()
                .filter(|e| e.column_group == Some(g))
                .map(|e| e.cursor.row)
                .max()
                .unwrap_or(main_row);

            // if group is contiguous from main..max and max is below main, shrink from bottom
            if max_row > main_row
                && self.group_contiguous_between(g, main_row, max_row)
                && self.remove_one_in_group_at_row(g, max_row)
            {
                continue;
            }

            // otherwise add one above for every cursor in the group
            let positions: Vec<(usize, usize)> = self
                .cursors
                .iter()
                .filter(|e| e.column_group == Some(g))
                .map(|e| (e.cursor.row, e.cursor.column))
                .collect();

            for (row, col) in positions {
                let mut c = Cursor::new();
                c.move_to(buffer, row.saturating_sub(1), col);

                let cursor_entry = CursorEntry::group(self.new_cursor_id(), &c, g);
                self.cursors.push(cursor_entry);
            }
        }

        self.sort_and_dedup_cursors();
    }

    fn add_cursor_below(&mut self, buffer: &Buffer) {
        self.ensure_all_in_groups();
        let groups = self.group_ids();
        let last_row = buffer.line_count().saturating_sub(1);

        for g in groups {
            let Some(main) = self.group_main(g) else {
                continue;
            };
            let main_row = main.cursor.row;

            // find topmost cursor in this group
            let min_row = self
                .cursors
                .iter()
                .filter(|e| e.column_group == Some(g))
                .map(|e| e.cursor.row)
                .min()
                .unwrap_or(main_row);

            // if group is contiguous from min..main and min is above main, shrink from top
            if min_row < main_row
                && self.group_contiguous_between(g, min_row, main_row)
                && self.remove_one_in_group_at_row(g, min_row)
            {
                continue;
            }

            // otherwise grow below
            let positions: Vec<(usize, usize)> = self
                .cursors
                .iter()
                .filter(|e| e.column_group == Some(g))
                .map(|e| (e.cursor.row, e.cursor.column))
                .collect();

            for (row, col) in positions {
                if row >= last_row {
                    continue;
                }
                let mut c = Cursor::new();
                c.move_to(buffer, row.saturating_add(1), col);

                let cursor_entry = CursorEntry::group(self.new_cursor_id(), &c, g);
                self.cursors.push(cursor_entry);
            }
        }

        self.sort_and_dedup_cursors();
    }

    fn add_cursor_at(&mut self, buffer: &Buffer, row: usize, col: usize) {
        let mut cursor = Cursor::new();
        cursor.move_to(buffer, row, col);

        let cursor_entry = CursorEntry::individual(self.new_cursor_id(), &cursor);
        self.cursors.push(cursor_entry);
        self.sort_and_dedup_cursors();
    }
}

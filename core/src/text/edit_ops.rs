use super::{
    Cursor, Edit, Editor, change_set::OpFlags, cursor_manager::CursorEntry, editor::DeleteResult,
};

impl Editor {
    pub fn insert_text(&mut self, text: &str) -> super::ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            let has_nl = text.contains('\n');

            let mut idxs: Vec<usize> = editor
                .delete_selection_preserve_cursors()
                .unwrap_or_else(|| editor.cursor_manager.cursor_byte_idxs(&editor.buffer));

            editor.for_each_cursor_rev(|editor, i| {
                editor.insert_at(text, i, &mut idxs, |pos: usize, editor: &mut Editor| {
                    editor.invalidate_insert_lines(pos, text, has_nl);
                });
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.widths.sync_line_widths(self.buffer.line_count());
        res
    }

    pub fn backspace(&mut self) -> super::ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if editor.delete_selection_if_active() {
                return;
            }

            let mut idxs: Vec<usize> = editor
                .cursor_manager
                .cursors
                .iter()
                .map(|c| c.cursor.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.backspace_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    pub fn delete(&mut self) -> super::ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if editor.delete_selection_if_active() {
                return;
            }

            let mut idxs: Vec<usize> = editor
                .cursor_manager
                .cursors
                .iter()
                .map(|c| c.cursor.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.delete_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    fn invalidate_insert_lines(&mut self, pos: usize, text: &str, has_nl: bool) {
        let row = self.buffer.byte_to_line(pos);
        self.widths.invalidate_line_width(row);

        if has_nl {
            let extra_lines = text.bytes().filter(|b| *b == b'\n').count();
            for offset in 1..=extra_lines {
                let line = row + offset;
                if line < self.buffer.line_count() {
                    self.widths.invalidate_line_width(line);
                }
            }
        }
    }

    fn apply_delete_to_idxs(&self, start: usize, end: usize, idxs: &mut [usize], caret_i: usize) {
        let delta = end - start;

        (0..idxs.len()).for_each(|j| {
            let idx = idxs[j];
            if idx > end {
                idxs[j] = idx - delta;
            } else if idx > start {
                idxs[j] = start;
            }
        });

        idxs[caret_i] = start;
    }

    fn delete_selection_preserve_cursors(&mut self) -> Option<Vec<usize>> {
        if !self.selection_manager.selection.is_active() {
            return None;
        }

        let a = self.selection_manager.selection.start.get_idx(&self.buffer);
        let b = self.selection_manager.selection.end.get_idx(&self.buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };
        let delta = end - start;

        let cursor_info: Vec<(CursorEntry, usize)> = self
            .cursor_manager
            .cursors
            .iter()
            .cloned()
            .map(|entry| {
                let idx = entry.cursor.get_idx(&self.buffer);
                (entry, idx)
            })
            .collect();
        let active_id = self
            .cursor_manager
            .cursors
            .get(self.cursor_manager.active_cursor_index)
            .map(|c| c.id);

        let deleted = self.buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection_manager.clear_selection();
        self.widths
            .clear_and_rebuild_line_widths(self.buffer.line_count());

        let mut new_cursors: Vec<CursorEntry> = Vec::new();
        for (entry, idx) in cursor_info {
            let new_idx = if idx <= start {
                idx
            } else if idx >= end {
                idx - delta
            } else {
                continue;
            };

            let (row, col) = self.buffer.byte_to_row_col(new_idx);
            let mut cursor = entry.cursor.clone();
            cursor.move_to(&self.buffer, row, col);
            new_cursors.push(CursorEntry {
                id: entry.id,
                cursor,
                column_group: entry.column_group,
            });
        }

        if new_cursors.is_empty() {
            let mut cursor = Cursor::new();
            let (row, col) = self.buffer.byte_to_row_col(start);
            cursor.move_to(&self.buffer, row, col);
            new_cursors.push(CursorEntry::individual(
                self.cursor_manager.new_cursor_id(),
                &cursor,
            ));
        }

        self.cursor_manager.set_cursors(new_cursors);
        self.cursor_manager.sort_and_dedup_cursors();

        if let Some(id) = active_id {
            self.cursor_manager.set_active_cursor_index(
                self.cursor_manager
                    .cursors
                    .iter()
                    .position(|c| c.id == id)
                    .unwrap_or(0),
            );
        } else {
            self.cursor_manager.set_active_cursor_index(0);
        }

        Some(self.cursor_manager.cursor_byte_idxs(&self.buffer))
    }

    fn insert_at(
        &mut self,
        text: &str,
        i: usize,
        idxs: &mut [usize],
        invalidate_lines: impl FnOnce(usize, &mut Editor),
    ) {
        let pos = idxs[i];

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let ins_len = text.len();

        (0..idxs.len()).for_each(|j| {
            if idxs[j] > pos {
                idxs[j] += ins_len;
            } else if idxs[j] == pos {
                idxs[j] = pos + ins_len;
            }
        });

        invalidate_lines(pos, self);
    }

    fn backspace_impl(&mut self, i: usize, idxs: &mut [usize]) -> DeleteResult {
        let pos = idxs[i];
        if pos == 0 {
            return DeleteResult::Text { invalidate: None };
        }

        let start = if pos >= 2 && self.buffer.get_text_range(pos - 2, pos) == "\r\n" {
            pos - 2
        } else {
            pos - 1
        };
        let end = pos;

        let deleted = self.buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted: deleted.clone(),
        });

        self.apply_delete_to_idxs(start, end, idxs, i);

        if deleted.contains('\n') {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }

    fn delete_impl(&mut self, i: usize, idxs: &mut [usize]) -> DeleteResult {
        let start = idxs[i];
        if start >= self.buffer.byte_len().saturating_sub(1) {
            return DeleteResult::Text { invalidate: None };
        }

        let mut end = start + 1;
        if self.buffer.get_text_range(start, end) == "\r"
            && start + 2 <= self.buffer.byte_len()
            && self.buffer.get_text_range(start, start + 2) == "\r\n"
        {
            end = start + 2;
        }

        let deleted = self.buffer.delete_range(start, end);
        if deleted.is_empty() {
            return DeleteResult::Text { invalidate: None };
        }

        self.record_edit(Edit::Delete {
            start,
            end,
            deleted: deleted.clone(),
        });

        self.apply_delete_to_idxs(start, end, idxs, i);

        if deleted.contains('\n') {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }
}

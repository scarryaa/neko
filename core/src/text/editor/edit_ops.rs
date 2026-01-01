use super::{ChangeSet, DeleteResult, Edit, OpFlags};
use crate::{Buffer, Cursor, CursorEntry, Editor};

impl Editor {
    pub fn insert_text(&mut self, buffer: &mut Buffer, text: &str) -> ChangeSet {
        let res = self.with_op(
            buffer,
            true,
            OpFlags::BufferViewportWidths,
            |editor, buffer| {
                let has_nl = text.contains('\n');

                let mut idxs: Vec<usize> = editor
                    .delete_selection_preserve_cursors(buffer)
                    .unwrap_or_else(|| editor.cursor_manager().cursor_byte_idxs(buffer));

                editor.for_each_cursor_rev(|editor, i| {
                    editor.insert_at(
                        buffer,
                        text,
                        i,
                        &mut idxs,
                        |pos: usize, editor: &mut Editor, buffer: &mut Buffer| {
                            editor.invalidate_insert_lines(buffer, pos, text, has_nl);
                        },
                    );
                });

                let cursors = editor.cursor_manager_mut().cursors_mut();
                for (c, &idx) in cursors.iter_mut().zip(idxs.iter()) {
                    let (row, col) = buffer.byte_to_row_col(idx);
                    c.cursor.move_to(buffer, row, col);
                }
            },
        );

        let line_count = buffer.line_count();
        self.widths_mut().sync_line_widths(line_count);
        res
    }

    pub fn backspace(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let res = self.with_op(
            buffer,
            true,
            OpFlags::BufferViewportWidths,
            |editor, buffer| {
                if editor.delete_selection_if_active(buffer) {
                    return;
                }

                let cursors = editor.cursor_manager().cursors();
                let mut idxs: Vec<usize> =
                    cursors.iter().map(|c| c.cursor.get_idx(buffer)).collect();

                editor.for_each_cursor_rev(|editor, i| {
                    let res = editor.backspace_impl(buffer, i, &mut idxs);
                    editor.apply_delete_result(buffer, i, res);
                });

                let cursors = editor.cursor_manager_mut().cursors_mut();
                for (c, &idx) in cursors.iter_mut().zip(idxs.iter()) {
                    let (row, col) = buffer.byte_to_row_col(idx);
                    c.cursor.move_to(buffer, row, col);
                }
            },
        );

        let line_count = buffer.line_count();
        self.widths_mut().sync_line_widths(line_count);
        res
    }

    pub fn delete(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let res = self.with_op(
            buffer,
            true,
            OpFlags::BufferViewportWidths,
            |editor, buffer| {
                if editor.delete_selection_if_active(buffer) {
                    return;
                }

                let cursors = editor.cursor_manager().cursors();
                let mut idxs: Vec<usize> =
                    cursors.iter().map(|c| c.cursor.get_idx(buffer)).collect();

                editor.for_each_cursor_rev(|editor, i| {
                    let res = editor.delete_impl(buffer, i, &mut idxs);
                    editor.apply_delete_result(buffer, i, res);
                });

                let cursors = editor.cursor_manager_mut().cursors_mut();
                for (c, &idx) in cursors.iter_mut().zip(idxs.iter()) {
                    let (row, col) = buffer.byte_to_row_col(idx);
                    c.cursor.move_to(buffer, row, col);
                }
            },
        );

        let line_count = buffer.line_count();
        self.widths_mut().sync_line_widths(line_count);
        res
    }

    fn invalidate_insert_lines(&mut self, buffer: &Buffer, pos: usize, text: &str, has_nl: bool) {
        let row = buffer.byte_to_line(pos);
        self.widths_mut().invalidate_line_width(row);

        if has_nl {
            let extra_lines = text.bytes().filter(|b| *b == b'\n').count();
            for offset in 1..=extra_lines {
                let line = row + offset;
                if line < buffer.line_count() {
                    self.widths_mut().invalidate_line_width(line);
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

    fn delete_selection_preserve_cursors(&mut self, buffer: &mut Buffer) -> Option<Vec<usize>> {
        if !self.selection_manager().selection.is_active() {
            return None;
        }

        let a = self.selection_manager().selection.start.get_idx(buffer);
        let b = self.selection_manager().selection.end.get_idx(buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };
        let delta = end - start;

        let cursor_info: Vec<(CursorEntry, usize)> = self
            .cursor_manager()
            .cursors
            .iter()
            .cloned()
            .map(|entry| {
                let idx = entry.cursor.get_idx(buffer);
                (entry, idx)
            })
            .collect();
        let active_id = self
            .cursor_manager()
            .cursors
            .get(self.cursor_manager().active_cursor_index)
            .map(|c| c.id);

        let deleted = buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection_manager_mut().clear_selection();

        let line_count = buffer.line_count();
        self.widths_mut().clear_and_rebuild_line_widths(line_count);

        let mut new_cursors: Vec<CursorEntry> = Vec::new();
        for (entry, idx) in cursor_info {
            let new_idx = if idx <= start {
                idx
            } else if idx >= end {
                idx - delta
            } else {
                continue;
            };

            let (row, col) = buffer.byte_to_row_col(new_idx);
            let mut cursor = entry.cursor.clone();
            cursor.move_to(buffer, row, col);
            new_cursors.push(CursorEntry {
                id: entry.id,
                cursor,
                column_group: entry.column_group,
            });
        }

        if new_cursors.is_empty() {
            let mut cursor = Cursor::new();
            let (row, col) = buffer.byte_to_row_col(start);
            cursor.move_to(buffer, row, col);
            new_cursors.push(CursorEntry::individual(
                self.cursor_manager_mut().new_cursor_id(),
                &cursor,
            ));
        }

        self.cursor_manager_mut().set_cursors(new_cursors);
        self.cursor_manager_mut().sort_and_dedup_cursors();

        if let Some(id) = active_id {
            let new_index = self
                .cursor_manager()
                .cursors
                .iter()
                .position(|c| c.id == id)
                // TODO(scarlet): Convert this to CursorId
                .unwrap_or(0);
            self.cursor_manager_mut().set_active_cursor_index(new_index);
        } else {
            self.cursor_manager_mut().set_active_cursor_index(0);
        }

        Some(self.cursor_manager().cursor_byte_idxs(buffer))
    }

    fn insert_at(
        &mut self,
        buffer: &mut Buffer,
        text: &str,
        i: usize,
        idxs: &mut [usize],
        mut invalidate_lines: impl FnMut(usize, &mut Editor, &mut Buffer),
    ) {
        let pos = idxs[i];

        buffer.insert(pos, text);
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

        invalidate_lines(pos, self, buffer);
    }

    fn backspace_impl(
        &mut self,
        buffer: &mut Buffer,
        i: usize,
        idxs: &mut [usize],
    ) -> DeleteResult {
        let pos = idxs[i];

        if pos == 0 {
            let len = buffer.byte_len();

            // "\n"
            if len == 1 && buffer.get_text_range(0, 1) == "\n" {
                let deleted = buffer.delete_range(0, 1);
                self.record_edit(Edit::Delete {
                    start: 0,
                    end: 1,
                    deleted: deleted.clone(),
                });

                (0..idxs.len()).for_each(|j| {
                    idxs[j] = 0;
                });

                return DeleteResult::Newline {
                    row: 0,
                    invalidate: Some(0),
                };
            }

            // "\r\n"
            if len == 2 && buffer.get_text_range(0, 2) == "\r\n" {
                let deleted = buffer.delete_range(0, 2);
                self.record_edit(Edit::Delete {
                    start: 0,
                    end: 2,
                    deleted: deleted.clone(),
                });

                (0..idxs.len()).for_each(|j| {
                    idxs[j] = 0;
                });

                return DeleteResult::Newline {
                    row: 0,
                    invalidate: Some(0),
                };
            }

            return DeleteResult::Text { invalidate: None };
        }

        let start = if pos >= 2 && buffer.get_text_range(pos - 2, pos) == "\r\n" {
            pos - 2
        } else {
            pos - 1
        };
        let end = pos;

        let deleted = buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted: deleted.clone(),
        });

        self.apply_delete_to_idxs(start, end, idxs, i);

        let row = buffer.byte_to_line(start);
        if deleted.contains('\n') {
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }

    fn delete_impl(&mut self, buffer: &mut Buffer, i: usize, idxs: &mut [usize]) -> DeleteResult {
        let start = idxs[i];
        let mut end = start + 1;
        if buffer.get_text_range(start, end) == "\r"
            && start + 2 <= buffer.byte_len()
            && buffer.get_text_range(start, start + 2) == "\r\n"
        {
            end = start + 2;
        }

        let deleted = buffer.delete_range(start, end);
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
            let row = buffer.byte_to_line(start);
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            let row = buffer.byte_to_line(start);
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }
}

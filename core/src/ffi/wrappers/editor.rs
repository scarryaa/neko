use crate::{
    Buffer, Editor,
    ffi::{AddCursorDirectionFfi, ChangeSetFfi, CursorPosition, Selection},
};

impl Editor {
    pub(crate) fn get_text(&self, buffer: &Buffer) -> String {
        buffer.get_text()
    }

    pub(crate) fn get_line(&self, buffer: &Buffer, line_idx: usize) -> String {
        buffer.get_line(line_idx)
    }

    pub(crate) fn get_line_count(&self, buffer: &Buffer) -> usize {
        buffer.line_count()
    }

    pub(crate) fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition> {
        self.cursors()
            .iter()
            .map(|c| CursorPosition {
                row: c.cursor.row,
                col: c.cursor.column,
            })
            .collect()
    }

    pub(crate) fn get_selection(&self) -> Selection {
        let s = self.selection().clone();
        let start = s.start.clone();
        let end = s.end.clone();
        let anchor = s.anchor.clone();
        let active = s.is_active();

        Selection {
            start: CursorPosition {
                row: start.row,
                col: start.column,
            },
            end: CursorPosition {
                row: end.row,
                col: end.column,
            },
            anchor: CursorPosition {
                row: anchor.row,
                col: anchor.column,
            },
            active,
        }
    }

    pub(crate) fn copy(self: &Editor, buffer: &Buffer) -> String {
        let selection = self.selection().clone();

        if selection.is_active() {
            buffer.get_text_range(
                selection.start.get_idx(buffer),
                selection.end.get_idx(buffer),
            )
        } else {
            "".to_string()
        }
    }

    pub(crate) fn paste(&mut self, buffer: &mut Buffer, text: &str) -> ChangeSetFfi {
        self.insert_text(buffer, text).into()
    }

    pub(crate) fn insert_text_wrapper(&mut self, buffer: &mut Buffer, text: &str) -> ChangeSetFfi {
        self.insert_text(buffer, text).into()
    }

    pub(crate) fn move_to_wrapper(
        &mut self,
        buffer: &mut Buffer,
        row: usize,
        col: usize,
        clear_selection: bool,
    ) -> ChangeSetFfi {
        self.move_to(buffer, row, col, clear_selection).into()
    }

    pub(crate) fn select_to_wrapper(
        &mut self,
        buffer: &mut Buffer,
        row: usize,
        col: usize,
    ) -> ChangeSetFfi {
        self.select_to(buffer, row, col).into()
    }

    pub(crate) fn select_word_wrapper(
        &mut self,
        buffer: &mut Buffer,
        row: usize,
        col: usize,
    ) -> ChangeSetFfi {
        self.select_word(buffer, row, col).into()
    }

    pub(crate) fn select_word_drag_wrapper(
        &mut self,
        buffer: &mut Buffer,
        anchor_start_row: usize,
        anchor_start_col: usize,
        anchor_end_row: usize,
        anchor_end_col: usize,
        row: usize,
        col: usize,
    ) -> ChangeSetFfi {
        self.select_word_drag(
            buffer,
            anchor_start_row,
            anchor_start_col,
            anchor_end_row,
            anchor_end_col,
            row,
            col,
        )
        .into()
    }

    pub(crate) fn select_line_drag_wrapper(
        &mut self,
        buffer: &mut Buffer,
        anchor_row: usize,
        row: usize,
    ) -> ChangeSetFfi {
        self.select_line_drag(buffer, anchor_row, row).into()
    }

    pub(crate) fn move_left_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.move_left(buffer).into()
    }

    pub(crate) fn move_right_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.move_right(buffer).into()
    }

    pub(crate) fn move_up_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.move_up(buffer).into()
    }

    pub(crate) fn move_down_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.move_down(buffer).into()
    }

    pub(crate) fn insert_newline_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.insert_text(buffer, "\n").into()
    }

    pub(crate) fn insert_tab_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.insert_text(buffer, "\t").into()
    }

    pub(crate) fn backspace_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.backspace(buffer).into()
    }

    pub(crate) fn delete_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.delete(buffer).into()
    }

    pub(crate) fn select_all_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.select_all(buffer).into()
    }

    pub(crate) fn select_left_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.select_left(buffer).into()
    }

    pub(crate) fn select_right_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.select_right(buffer).into()
    }

    pub(crate) fn select_up_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.select_up(buffer).into()
    }

    pub(crate) fn select_down_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.select_down(buffer).into()
    }

    pub(crate) fn clear_selection_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.clear_selection(buffer).into()
    }

    pub(crate) fn clear_cursors_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.clear_cursors(buffer).into()
    }

    pub(crate) fn undo_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.undo(buffer).into()
    }

    pub(crate) fn redo_wrapper(&mut self, buffer: &mut Buffer) -> ChangeSetFfi {
        self.redo(buffer).into()
    }

    pub(crate) fn get_max_width(&self) -> f64 {
        self.widths().max_width()
    }

    pub(crate) fn add_cursor_wrapper(
        self: &mut Editor,
        buffer: &Buffer,
        direction: AddCursorDirectionFfi,
    ) {
        self.add_cursor(buffer, direction.into());
    }

    pub(crate) fn get_last_added_cursor(&self) -> CursorPosition {
        self.last_added_cursor().into()
    }
}

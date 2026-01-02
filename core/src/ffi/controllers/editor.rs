use crate::{
    AppState, Buffer, Editor, ViewId,
    ffi::{AddCursorDirectionFfi, ChangeSetFfi, CursorPosition, Selection},
};
use std::{cell::RefCell, rc::Rc};

pub struct EditorController {
    /// A reference to the main app.
    pub(crate) app_state: Rc<RefCell<AppState>>,
    /// The view id needed to find the desired editor instance.
    #[allow(dead_code)]
    pub(crate) view_id: ViewId,
}

impl EditorController {
    fn access_mut<F, R>(&mut self, f: F) -> R
    where
        F: FnOnce(&mut Editor, &mut Buffer) -> R,
    {
        self.app_state
            .borrow_mut()
            .with_view_and_document_mut(self.view_id, |view, document| {
                f(view.editor_mut(), &mut document.buffer)
            })
            .expect("Unable to access the specified Editor or Buffer.")
    }

    fn access<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&Editor, &Buffer) -> R,
    {
        self.app_state
            .borrow()
            .with_view_and_document(self.view_id, |view, document| {
                f(view.editor(), &document.buffer)
            })
            .expect("Unable to access the specified Editor or Buffer.")
    }

    pub fn get_text(&self) -> String {
        self.access(|_, buffer| buffer.get_text())
    }

    pub fn get_line(&self, line_idx: usize) -> String {
        self.access(|_, buffer| buffer.get_line(line_idx))
    }

    pub fn get_line_count(&self) -> usize {
        self.access(|_, buffer| buffer.line_count())
    }

    pub fn get_cursor_positions(&self) -> Vec<CursorPosition> {
        self.access(|editor, _| {
            editor
                .cursors()
                .iter()
                .map(|c| CursorPosition {
                    row: c.cursor.row,
                    col: c.cursor.column,
                })
                .collect()
        })
    }

    pub fn update_line_width(&mut self, line_idx: usize, line_width: f64) {
        self.access_mut(|editor, _| editor.update_line_width(line_idx, line_width))
    }

    pub fn buffer_is_empty(&self) -> bool {
        self.access(|_, buffer| buffer.is_empty())
    }

    pub fn get_selection(&self) -> Selection {
        self.access(|editor, _| {
            let s = editor.selection().clone();
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
        })
    }

    pub fn copy(&self) -> String {
        self.access(|editor, buffer| {
            let selection = editor.selection().clone();

            if selection.is_active() {
                buffer.get_text_range(
                    selection.start.get_idx(buffer),
                    selection.end.get_idx(buffer),
                )
            } else {
                "".to_string()
            }
        })
    }

    pub fn paste(&mut self, text: &str) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.insert_text(buffer, text).into())
    }

    pub fn insert_text(&mut self, text: &str) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.insert_text(buffer, text).into())
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear_selection: bool) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.move_to(buffer, row, col, clear_selection).into())
    }

    pub fn select_to(&mut self, row: usize, col: usize) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_to(buffer, row, col).into())
    }

    pub fn select_word(&mut self, row: usize, col: usize) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_word(buffer, row, col).into())
    }

    #[allow(clippy::too_many_arguments)]
    pub fn select_word_drag(
        &mut self,
        anchor_start_row: usize,
        anchor_start_col: usize,
        anchor_end_row: usize,
        anchor_end_col: usize,
        row: usize,
        col: usize,
    ) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| {
            editor
                .select_word_drag(
                    buffer,
                    anchor_start_row,
                    anchor_start_col,
                    anchor_end_row,
                    anchor_end_col,
                    row,
                    col,
                )
                .into()
        })
    }

    pub fn select_line_drag(&mut self, anchor_row: usize, row: usize) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_line_drag(buffer, anchor_row, row).into())
    }

    pub fn move_left(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.move_left(buffer))
            .into()
    }

    pub fn move_right(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.move_right(buffer))
            .into()
    }

    pub fn move_up(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.move_up(buffer))
            .into()
    }

    pub fn move_down(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.move_down(buffer))
            .into()
    }

    pub fn insert_newline(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.insert_text(buffer, "\n"))
            .into()
    }

    pub fn insert_tab(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.insert_text(buffer, "\t"))
            .into()
    }

    pub fn backspace(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.backspace(buffer))
            .into()
    }

    pub fn delete(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.delete(buffer))
            .into()
    }

    pub fn select_all(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_all(buffer))
            .into()
    }

    pub fn select_left(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_left(buffer))
            .into()
    }

    pub fn select_right(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_right(buffer))
            .into()
    }

    pub fn select_up(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_up(buffer))
            .into()
    }

    pub fn select_down(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_down(buffer))
            .into()
    }

    pub fn clear_selection(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.clear_selection(buffer))
            .into()
    }

    pub fn clear_cursors(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.clear_cursors(buffer))
            .into()
    }

    pub fn undo(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.undo(buffer)).into()
    }

    pub fn redo(&mut self) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.redo(buffer)).into()
    }

    pub fn get_max_width(&self) -> f64 {
        self.access(|editor, _| editor.widths().max_width())
    }

    pub fn add_cursor(&mut self, direction: AddCursorDirectionFfi) {
        self.access_mut(|editor, buffer| editor.add_cursor(buffer, direction.into()))
    }

    pub fn get_last_added_cursor(&self) -> CursorPosition {
        self.access(|editor, _| editor.last_added_cursor()).into()
    }

    pub fn needs_width_measurement(&self, line_idx: usize) -> bool {
        self.access(|editor, _| editor.needs_width_measurement(line_idx))
    }

    pub fn remove_cursor(&mut self, row: usize, col: usize) {
        self.access_mut(|editor, _| editor.remove_cursor(row, col))
    }

    pub fn cursor_exists_at(&self, row: usize, col: usize) -> bool {
        self.access(|editor, _| editor.cursor_exists_at(row, col))
    }

    pub fn has_active_selection(&self) -> bool {
        self.access(|editor, _| editor.has_active_selection())
    }

    pub fn cursor_exists_at_row(&self, row: usize) -> bool {
        self.access(|editor, _| editor.cursor_exists_at_row(row))
    }

    pub fn active_cursor_index(&self) -> usize {
        self.access(|editor, _| editor.active_cursor_index())
    }

    pub fn number_of_selections(&self) -> usize {
        self.access(|editor, _| editor.number_of_selections())
    }

    pub fn line_length(&self, row: usize) -> usize {
        self.access(|editor, buffer| editor.line_length(buffer, row))
    }

    pub fn lines(&self) -> Vec<String> {
        self.access(|editor, buffer| editor.lines(buffer))
    }
}

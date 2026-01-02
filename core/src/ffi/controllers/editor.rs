use crate::{AppState, Buffer, Editor, ViewId, ffi::ChangeSetFfi};
use std::{cell::RefCell, rc::Rc};

pub struct EditorController {
    /// A reference to the main app.
    pub(crate) app_state: Rc<RefCell<AppState>>,
    /// The view id needed to find the desired editor instance.
    #[allow(dead_code)]
    pub(crate) view_id: ViewId,
}

impl EditorController {
    fn access_mut<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&mut Editor, &mut Buffer) -> R,
    {
        let mut app = self.app_state.borrow_mut();
        app.with_active_view_mut(|view, document| f(view.editor_mut(), &mut document.buffer))
            .unwrap()
    }

    pub fn select_word(&mut self, row: usize, column: usize) -> ChangeSetFfi {
        self.access_mut(|editor, buffer| editor.select_word(buffer, row, column).into())
    }
}

use std::{cell::RefCell, rc::Rc};

use crate::AppState;

pub struct FileTreeController {
    pub(crate) app_state: Rc<RefCell<AppState>>,
}

impl FileTreeController {
    pub fn toggle_expanded(&mut self, path: &str) {
        self.app_state
            .borrow_mut()
            .get_file_tree_mut()
            .toggle_expanded(path);
    }
}

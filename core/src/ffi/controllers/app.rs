use crate::{AppState, ConfigManager, DocumentError, ffi::EditorHandle};
use std::{cell::RefCell, path::PathBuf, rc::Rc};

pub struct AppController {
    pub(crate) app_state: Rc<RefCell<AppState>>,
}

impl AppController {
    pub fn new(config_manager: &ConfigManager, root_path: String) -> Box<AppController> {
        let state =
            AppState::new(config_manager, Some(&root_path)).expect("Unable to create new AppState");
        Box::new(AppController {
            app_state: Rc::new(RefCell::new(state)),
        })
    }

    pub fn get_active_editor_mut(&self) -> Box<EditorHandle> {
        let app_state = self.app_state.borrow();
        let view_id = app_state
            .get_view_manager()
            .active_view()
            .expect("Unable to get active view id");

        Box::new(EditorHandle {
            app_state: self.app_state.clone(),
            view_id,
        })
    }

    pub fn open_file(&self, path: String, add_to_history: bool) -> Result<u64, DocumentError> {
        self.app_state
            .borrow_mut()
            .ensure_tab_for_path(&PathBuf::from(path), add_to_history)
            .map(|tab_id| tab_id.into())
    }
}

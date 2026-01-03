use crate::{
    AppState, ConfigManager,
    ffi::{DocumentErrorFfi, OpenTabResultFfi, TabController},
};
use std::{cell::RefCell, path::Path, rc::Rc};

use super::{CommandController, EditorController, FileTreeController};

// pub fn new_app_state(
//     root_path: &str,
//     config_manager: &ConfigManager,
// ) -> std::io::Result<Box<AppState>> {
//     let path_opt = if root_path.is_empty() {
//         None
//     } else {
//         Some(root_path)
//     };
//
//     let state = AppState::new(config_manager, path_opt)
//         .map_err(|e| std::io::Error::other(format!("Failed to initialize app state: {e}")))?;
//
//     Ok(Box::new(state))
// }

pub fn new_app_controller(config_manager: &ConfigManager, root_path: String) -> Box<AppController> {
    AppController::new(config_manager, root_path)
}

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

    pub fn tab_controller(&self) -> Box<TabController> {
        Box::new(TabController {
            app_state: self.app_state.clone(),
        })
    }

    pub fn file_tree_controller(&self) -> Box<FileTreeController> {
        Box::new(FileTreeController {
            app_state: self.app_state.clone(),
        })
    }

    pub fn editor_controller(&self) -> Box<EditorController> {
        let app_state = self.app_state.borrow();
        let view_id = app_state
            .get_view_manager()
            .active_view()
            .expect("Unable to get active view id");

        Box::new(EditorController {
            app_state: self.app_state.clone(),
            view_id,
        })
    }

    pub fn command_controller(&self) -> Box<CommandController> {
        Box::new(CommandController {
            app_state: self.app_state.clone(),
        })
    }

    pub fn ensure_tab_for_path(
        &mut self,
        path: &str,
        add_to_history: bool,
    ) -> Result<OpenTabResultFfi, DocumentErrorFfi> {
        self.app_state
            .borrow_mut()
            .ensure_tab_for_path(Path::new(path), add_to_history)
            .map(|result| result.into())
            .map_err(DocumentErrorFfi::from)
    }

    pub fn save_document(&mut self, id: u64) -> bool {
        self.app_state.borrow_mut().save_document(id.into()).is_ok()
    }

    pub fn save_document_as(&mut self, id: u64, path: &str) -> bool {
        self.app_state
            .borrow_mut()
            .save_document_as(id.into(), Path::new(path))
            .is_ok()
    }
}

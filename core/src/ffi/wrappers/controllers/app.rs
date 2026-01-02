use crate::{ConfigManager, ffi::AppController};

pub fn new_app_controller(config_manager: &ConfigManager, root_path: String) -> Box<AppController> {
    AppController::new(config_manager, root_path)
}

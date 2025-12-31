use crate::{ConfigManager, ffi::ConfigSnapshotFfi};

pub(crate) fn new_config_manager() -> std::io::Result<Box<ConfigManager>> {
    let config_manager = ConfigManager::new();

    Ok(Box::new(config_manager))
}

impl ConfigManager {
    pub(crate) fn get_snapshot_wrapper(&self) -> ConfigSnapshotFfi {
        self.get_snapshot().into()
    }

    pub(crate) fn apply_snapshot_wrapper(&self, snap: ConfigSnapshotFfi) {
        self.update(|c| {
            c.editor.font_size = snap.editor.font_size as usize;
            c.editor.font_family = snap.editor.font_family;

            c.file_explorer.font_size = snap.file_explorer.font_size as usize;
            c.file_explorer.font_family = snap.file_explorer.font_family;

            c.file_explorer.directory = if snap.file_explorer.directory_present {
                Some(snap.file_explorer.directory)
            } else {
                None
            };

            c.file_explorer.shown = snap.file_explorer.shown;
            c.file_explorer.width = snap.file_explorer.width as usize;
            c.file_explorer.right = snap.file_explorer.right;

            c.current_theme = snap.current_theme;
        });
    }

    pub(crate) fn get_config_path_wrapper(&self) -> String {
        if let Some(s) = ConfigManager::get_config_path().to_str() {
            s.to_string()
        } else {
            "".to_string()
        }
    }

    pub(crate) fn save_config_wrapper(&mut self) -> bool {
        self.save().is_ok()
    }
}

pub mod shortcuts_manager;
pub use shortcuts_manager::ShortcutsManager;

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Shortcut {
    // TODO: Convert to enum?
    pub key: String,
    pub key_combo: String,
}

impl Shortcut {
    pub fn new(key: String, key_combo: String) -> Self {
        Self { key, key_combo }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Shortcuts {
    shortcuts: Vec<Shortcut>,
}

impl Shortcuts {
    pub fn get(&self, key: &str) -> Option<&Shortcut> {
        self.shortcuts.iter().find(|s| s.key == key)
    }
}

impl Default for Shortcuts {
    fn default() -> Self {
        let open = Shortcut::new("Tab::Open".into(), "Ctrl+O".into());
        let save = Shortcut::new("Tab::Save".into(), "Ctrl+S".into());
        let close_tab = Shortcut::new("Tab::Close".into(), "Ctrl+W".into());
        let new_tab = Shortcut::new("Tab::New".into(), "Ctrl+T".into());

        let shortcuts = vec![open, save, close_tab, new_tab];

        Shortcuts { shortcuts }
    }
}

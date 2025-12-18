pub mod shortcuts_manager;
pub use shortcuts_manager::ShortcutsManager;

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Shortcut {
    // TODO: Convert to enum?
    pub key: String,
    pub action: String,
}

impl Shortcut {
    pub fn new(key: String, action: String) -> Self {
        Self { key, action }
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
        let open = Shortcut::new("open".into(), "Ctrl+O".into());
        let save = Shortcut::new("save".into(), "Ctrl+S".into());
        let close = Shortcut::new("close".into(), "Ctrl+W".into());

        let shortcuts = vec![open, save, close];

        Shortcuts { shortcuts }
    }
}

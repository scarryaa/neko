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

    pub fn upsert(&mut self, shortcut: Shortcut) {
        if let Some(existing) = self.shortcuts.iter_mut().find(|s| s.key == shortcut.key) {
            *existing = shortcut;
        } else {
            self.shortcuts.push(shortcut);
        }
    }

    pub fn extend<I: IntoIterator<Item = Shortcut>>(&mut self, shortcuts: I) {
        for s in shortcuts {
            self.upsert(s);
        }
    }

    pub fn as_vec(&self) -> Vec<Shortcut> {
        self.shortcuts.clone()
    }
}

impl Default for Shortcuts {
    fn default() -> Self {
        let open = Shortcut::new("Tab::Open".into(), "Ctrl+O".into());
        let save = Shortcut::new("Tab::Save".into(), "Ctrl+S".into());
        let save_as = Shortcut::new("Tab::SaveAs".into(), "Ctrl+Shift+S".into());
        let close_tab = Shortcut::new("Tab::Close".into(), "Ctrl+W".into());
        let force_close_tab = Shortcut::new("Tab::ForceClose".into(), "Ctrl+Shift+W".into());
        let new_tab = Shortcut::new("Tab::New".into(), "Ctrl+T".into());
        let next_tab = Shortcut::new("Tab::Next".into(), "Meta+Tab".into());
        let previous_tab = Shortcut::new("Tab::Previous".into(), "Meta+Shift+Tab".into());
        let jump_to = Shortcut::new("Cursor::JumpTo".into(), "Ctrl+G".into());
        let toggle_explorer = Shortcut::new("FileExplorer::Toggle".into(), "Ctrl+E".into());
        let focus_explorer = Shortcut::new("FileExplorer::Focus".into(), "Meta+H".into());
        let focus_editor = Shortcut::new("Editor::Focus".into(), "Meta+L".into());
        let open_config = Shortcut::new("Editor::OpenConfig".into(), "Ctrl+,".into());
        let open_command_palette = Shortcut::new("CommandPalette:Show".into(), "Ctrl+P".into());

        let shortcuts = vec![
            open,
            save,
            save_as,
            close_tab,
            force_close_tab,
            new_tab,
            next_tab,
            previous_tab,
            jump_to,
            toggle_explorer,
            focus_explorer,
            focus_editor,
            open_config,
            open_command_palette,
        ];

        Shortcuts { shortcuts }
    }
}

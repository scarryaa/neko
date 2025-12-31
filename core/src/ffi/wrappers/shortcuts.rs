use crate::{ShortcutsManager, ffi::Shortcut};

pub(crate) fn new_shortcuts_manager() -> std::io::Result<Box<ShortcutsManager>> {
    let shortcuts_manager = ShortcutsManager::new();

    Ok(Box::new(shortcuts_manager))
}

impl ShortcutsManager {
    pub(crate) fn get_shortcut_wrapper(&self, key: String) -> Shortcut {
        self.get_shortcut(&key)
            .map(Into::into)
            .unwrap_or_else(|| Shortcut {
                key,
                key_combo: String::new(),
            })
    }

    pub(crate) fn get_shortcuts_wrapper(&self) -> Vec<Shortcut> {
        self.get_all().into_iter().map(Into::into).collect()
    }

    pub(crate) fn add_shortcuts_wrapper(&self, shortcuts: Vec<Shortcut>) {
        let shortcuts = shortcuts
            .into_iter()
            .map(|s| crate::shortcuts::Shortcut::new(s.key, s.key_combo));

        self.add_shortcuts(shortcuts);
    }
}

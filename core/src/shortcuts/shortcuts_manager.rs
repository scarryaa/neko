use super::{Shortcut, Shortcuts};
use std::{fs, path::PathBuf, sync::RwLock};

pub struct ShortcutsManager {
    inner: RwLock<Shortcuts>,
    file_path: PathBuf,
}

impl Default for ShortcutsManager {
    fn default() -> Self {
        Self::new()
    }
}

impl ShortcutsManager {
    pub fn new() -> Self {
        let file_path = Self::get_user_shortcuts_path();
        let user_shortcuts = Self::load_from_disk(&file_path).unwrap_or_default();

        Self {
            inner: RwLock::new(user_shortcuts),
            file_path,
        }
    }

    fn get_user_shortcuts_path() -> PathBuf {
        let mut path = dirs::config_dir().unwrap_or_else(|| PathBuf::from("."));

        #[cfg(target_os = "macos")]
        {
            if let Some(home) = dirs::home_dir() {
                path = home.join(".config");
            }
        }
        path.push("neko");

        _ = fs::create_dir_all(&path);

        path.push("keymap.json");
        path
    }

    fn load_from_disk(path: &PathBuf) -> Option<Shortcuts> {
        if path.exists() {
            let content = fs::read_to_string(path).ok()?;
            serde_json::from_str(&content).ok()
        } else {
            None
        }
    }

    pub fn save(&self) -> Result<(), std::io::Error> {
        let shortcuts = self.inner.read().unwrap();
        let json = serde_json::to_string_pretty(&*shortcuts)?;
        fs::write(&self.file_path, json)?;
        Ok(())
    }

    pub fn get_shortcut(&self, key: &str) -> Option<Shortcut> {
        self.inner
            .read()
            .ok()
            .and_then(|shortcuts| shortcuts.get(key).cloned())
    }

    pub fn get_all(&self) -> Vec<Shortcut> {
        self.inner
            .read()
            .map(|shortcuts| shortcuts.as_vec())
            .unwrap_or_default()
    }

    pub fn add_shortcuts<I>(&self, new_shortcuts: I)
    where
        I: IntoIterator<Item = Shortcut>,
    {
        self.update(|shortcuts| shortcuts.extend(new_shortcuts));
    }

    pub fn get_snapshot(&self) -> Shortcuts {
        self.inner.read().unwrap().clone()
    }

    pub fn update<F>(&self, modify_fn: F)
    where
        F: FnOnce(&mut Shortcuts),
    {
        {
            let mut shortcuts = self.inner.write().unwrap();
            modify_fn(&mut shortcuts);
        }

        if let Err(e) = self.save() {
            eprintln!("Failed to save shortcuts: {e}");
        }
    }
}

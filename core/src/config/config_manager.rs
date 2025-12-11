use super::Config;
use std::{fs, path::PathBuf, sync::RwLock};

pub struct ConfigManager {
    inner: RwLock<Config>,
    file_path: PathBuf,
}

impl Default for ConfigManager {
    fn default() -> Self {
        Self::new()
    }
}

impl ConfigManager {
    pub fn new() -> Self {
        let file_path = Self::get_config_path();
        let config = Self::load_from_disk(&file_path).unwrap_or_default();

        Self {
            inner: RwLock::new(config),
            file_path,
        }
    }

    fn get_config_path() -> PathBuf {
        let mut path = dirs::config_dir().unwrap_or_else(|| PathBuf::from("."));

        #[cfg(target_os = "macos")]
        {
            if let Some(home) = dirs::home_dir() {
                path = home.join(".config");
            }
        }
        path.push("neko");

        if let Err(e) = fs::create_dir(&path) {
            eprintln!("Failed to create config directory: {e}");
        }

        path.push("settings.json");
        path
    }

    fn load_from_disk(path: &PathBuf) -> Option<Config> {
        if path.exists() {
            let content = fs::read_to_string(path).ok()?;
            serde_json::from_str(&content).ok()
        } else {
            None
        }
    }

    fn save(&self) -> Result<(), std::io::Error> {
        let config = self.inner.read().unwrap();
        let json = serde_json::to_string_pretty(&*config)?;
        fs::write(&self.file_path, json)?;
        Ok(())
    }

    pub fn get_snapshot(&self) -> Config {
        self.inner.read().unwrap().clone()
    }

    pub fn update<F>(&self, modify_fn: F)
    where
        F: FnOnce(&mut Config),
    {
        {
            let mut config = self.inner.write().unwrap();
            modify_fn(&mut config);
        }

        if let Err(e) = self.save() {
            eprintln!("Failed to save config: {e}");
        }
    }
}

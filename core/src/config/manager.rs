use crate::Config;
use std::{fs, path::PathBuf, sync::RwLock};

#[derive(Debug)]
pub struct ConfigManager {
    inner: RwLock<Config>,
    file_path: PathBuf,
}

impl Default for ConfigManager {
    fn default() -> Self {
        Self::new()
    }
}

// TODO(scarlet): Add validation for config schema? E.g. currently if the user adds a key under
// the jump section like 'invalid_key': 'unknown_command', nothing happens and it is ignored.
// Ideally display a warning in-editor.
impl ConfigManager {
    pub fn new() -> Self {
        let file_path = Self::get_config_path();
        let config = Self::load_from_disk(&file_path).unwrap_or_default();

        Self {
            inner: RwLock::new(config),
            file_path,
        }
    }

    pub fn get_config_path() -> PathBuf {
        let mut path = dirs::config_dir().unwrap_or_else(|| PathBuf::from("."));

        #[cfg(target_os = "macos")]
        {
            if let Some(home) = dirs::home_dir() {
                path = home.join(".config");
            }
        }
        path.push("neko");

        _ = fs::create_dir_all(&path);

        path.push("settings.json");
        path
    }

    fn load_from_disk(path: &PathBuf) -> Option<Config> {
        if !path.exists() {
            return None;
        }

        let content = fs::read_to_string(path).ok()?;

        match serde_json::from_str::<Config>(&content) {
            Ok(mut cfg) => {
                // Merge/backfill jump aliases
                cfg.ensure_jump_defaults();

                Some(cfg)
            }
            Err(e) => {
                eprintln!("Failed to parse config {}: {e}", path.display());
                None
            }
        }
    }

    pub fn reload_from_disk(&self) -> Result<(), std::io::Error> {
        match Self::load_from_disk(&self.file_path) {
            Some(cfg) => {
                let mut inner = self.inner.write().unwrap();
                *inner = cfg;
                Ok(())
            }
            None => Err(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                format!("Config file {} not found", self.file_path.display()),
            )),
        }
    }

    pub fn save(&self) -> Result<(), std::io::Error> {
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

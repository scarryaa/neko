pub mod config_manager;

pub use config_manager::ConfigManager;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    pub editor_font_size: usize,
    pub editor_font_family: String,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            editor_font_size: 15,
            editor_font_family: "IBM Plex Mono".to_string(),
        }
    }
}

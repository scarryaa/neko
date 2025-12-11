pub mod config_manager;

pub use config_manager::ConfigManager;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    pub editor_font_size: usize,
    pub editor_font_family: String,
    pub file_explorer_font_size: usize,
    pub file_explorer_font_family: String,
    pub file_explorer_directory: Option<String>,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            editor_font_size: 15,
            editor_font_family: "IBM Plex Mono".to_string(),
            file_explorer_font_family: "IBM Plex Sans".to_string(),
            file_explorer_font_size: 15,
            file_explorer_directory: None,
        }
    }
}

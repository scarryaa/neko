pub mod config_manager;

pub use config_manager::ConfigManager;
use serde::{Deserialize, Serialize};

use crate::Theme;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    pub editor_font_size: usize,
    pub editor_font_family: String,
    pub file_explorer_font_size: usize,
    pub file_explorer_font_family: String,
    pub file_explorer_directory: Option<String>,
    pub file_explorer_shown: bool,
    pub file_explorer_width: usize,
    pub file_explorer_right: bool,
    pub current_theme: String,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            editor_font_size: 15,
            editor_font_family: "IBM Plex Mono".to_string(),
            file_explorer_font_family: "IBM Plex Sans".to_string(),
            file_explorer_font_size: 15,
            file_explorer_directory: None,
            file_explorer_shown: true,
            file_explorer_width: 250,
            file_explorer_right: false,
            current_theme: Theme::default().name,
        }
    }
}

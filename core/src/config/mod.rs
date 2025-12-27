pub mod config_manager;

pub use config_manager::ConfigManager;
use serde::{Deserialize, Serialize};

use crate::Theme;

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
// TODO(scarlet): Scope this better into sections
// e.g.
//
// editor {
//  font_size: xyz,
// },
// explorer {
//  font_size: xyz,
// }
//
// TODO(scarlet): Update config on app open. Currently it only updates when config is modified by
// the app (e.g. toggling the file explorer)
pub struct Config {
    pub editor_font_size: usize,
    pub editor_font_family: String,
    pub editor_tab_history: bool,
    pub editor_auto_reopen_closed_tabs_in_history: bool,
    pub file_explorer_font_size: usize,
    pub file_explorer_font_family: String,
    pub file_explorer_directory: Option<String>,
    pub file_explorer_shown: bool,
    pub file_explorer_width: usize,
    pub file_explorer_right: bool,
    pub interface_font_family: String,
    pub interface_font_size: usize,
    pub terminal_font_family: String,
    pub terminal_font_size: usize,
    pub current_theme: String,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            editor_font_size: 15,
            editor_font_family: "IBM Plex Mono".to_string(),
            editor_tab_history: true,
            editor_auto_reopen_closed_tabs_in_history: true,
            file_explorer_font_family: "IBM Plex Sans".to_string(),
            file_explorer_font_size: 15,
            file_explorer_directory: None,
            file_explorer_shown: true,
            file_explorer_width: 250,
            file_explorer_right: false,
            interface_font_size: 15,
            interface_font_family: "IBM Plex Sans".to_string(),
            terminal_font_size: 15,
            terminal_font_family: "IBM Plex Mono".to_string(),
            current_theme: Theme::default().name,
        }
    }
}

use crate::Theme;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct EditorConfig {
    pub font_size: usize,
    pub font_family: String,
    pub switch_to_last_visited_tab_on_close: bool,
    pub auto_reopen_closed_tabs_in_history: bool,
}

impl Default for EditorConfig {
    fn default() -> Self {
        Self {
            font_size: 15,
            font_family: "IBM Plex Mono".into(),
            switch_to_last_visited_tab_on_close: true,
            auto_reopen_closed_tabs_in_history: true,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct FileExplorerConfig {
    pub font_size: usize,
    pub font_family: String,
    pub directory: Option<String>,
    pub shown: bool,
    pub width: usize,
    pub right: bool,
}

impl Default for FileExplorerConfig {
    fn default() -> Self {
        Self {
            font_size: 15,
            font_family: "IBM Plex Sans".to_string(),
            directory: None,
            shown: true,
            width: 250,
            right: false,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct InterfaceConfig {
    pub font_family: String,
    pub font_size: usize,
}

impl Default for InterfaceConfig {
    fn default() -> Self {
        Self {
            font_family: "IBM Plex Sans".to_string(),
            font_size: 15,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct TerminalConfig {
    pub font_family: String,
    pub font_size: usize,
}

impl Default for TerminalConfig {
    fn default() -> Self {
        Self {
            font_family: "IBM Plex Mono".to_string(),
            font_size: 15,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct JumpConfig {
    /// User-defined jump aliases; e.g. "db" -> "doc.start".
    pub aliases: HashMap<String, String>,
}

impl Default for JumpConfig {
    fn default() -> Self {
        let mut aliases = HashMap::new();

        aliases.insert("lb".into(), "line.start".into());
        aliases.insert("lm".into(), "line.middle".into());
        aliases.insert("le".into(), "line.end".into());

        aliases.insert("db".into(), "doc.start".into());
        aliases.insert("dm".into(), "doc.middle".into());
        aliases.insert("de".into(), "doc.end".into());
        aliases.insert("dh".into(), "doc.quarter".into());
        aliases.insert("dt".into(), "doc.three_quarters".into());

        Self { aliases }
    }
}

// TODO(scarlet): Update config on app open. Currently it only updates when config is modified by
// the app (e.g. toggling the file explorer)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct Config {
    pub editor: EditorConfig,
    pub file_explorer: FileExplorerConfig,
    pub interface: InterfaceConfig,
    pub terminal: TerminalConfig,
    pub current_theme: String,
    pub jump: JumpConfig,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            editor: EditorConfig::default(),
            file_explorer: FileExplorerConfig::default(),
            interface: InterfaceConfig::default(),
            terminal: TerminalConfig::default(),
            jump: JumpConfig::default(),
            current_theme: Theme::default().name,
        }
    }
}

impl Config {
    pub fn ensure_jump_defaults(&mut self) {
        let defaults = JumpConfig::default().aliases;
        for (k, v) in defaults {
            self.jump.aliases.entry(k).or_insert(v);
        }
    }
}

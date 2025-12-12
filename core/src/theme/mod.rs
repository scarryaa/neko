use std::collections::HashMap;

use serde::{Deserialize, Serialize};

pub mod theme_manager;
pub use theme_manager::ThemeManager;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Theme {
    pub name: String,
    pub colors: HashMap<String, String>,
}

impl Default for Theme {
    fn default() -> Self {
        let mut colors = HashMap::new();
        colors.insert("editor.background".to_string(), "#000000".to_string());
        colors.insert(
            "editor.gutter.background".to_string(),
            "#000000".to_string(),
        );
        colors.insert("editor.foreground".to_string(), "#d4d4d4".to_string());
        colors.insert("sidebar.background".to_string(), "#000000".to_string());
        colors.insert("ui.foreground".to_string(), "#d4d4d4".to_string());
        colors.insert("ui.foreground.muted".to_string(), "#a4a4a4".to_string());
        colors.insert("ui.background".to_string(), "#000000".to_string());
        colors.insert("ui.background.hover".to_string(), "#2c2c2c".to_string());
        colors.insert("ui.accent".to_string(), "#a589d1".to_string());
        colors.insert("ui.accent.hover".to_string(), "#bca3e0".to_string());
        colors.insert("ui.accent.pressed".to_string(), "#8e72b8".to_string());
        colors.insert("ui.accent.muted".to_string(), "#4d3e66".to_string());
        colors.insert("ui.border".to_string(), "#3c3c3c".to_string());
        colors.insert(
            "titlebar.button.foreground".to_string(),
            "#a0a0a0".to_string(),
        );
        colors.insert("titlebar.button.hover".to_string(), "#131313".to_string());
        colors.insert("titlebar.button.pressed".to_string(), "#222222".to_string());
        colors.insert("tab.active".to_string(), "#000000".to_string());
        colors.insert("tab.inactive".to_string(), "#202020".to_string());
        colors.insert("tab.hover".to_string(), "#101010".to_string());

        Self {
            name: "Default Dark".to_string(),
            colors,
        }
    }
}

impl Theme {
    pub fn get_color(&self, key: &str) -> &str {
        self.colors
            .get(key)
            .map(|s| s.as_str())
            .unwrap_or("#ff00ff")
    }
}

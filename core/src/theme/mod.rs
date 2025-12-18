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
        Self::dark_theme()
    }
}

impl Theme {
    pub fn get_color(&self, key: &str) -> &str {
        self.colors
            .get(key)
            .map(|s| s.as_str())
            .unwrap_or("#ff00ff")
    }

    pub fn get_theme(&self, key: &str) -> Option<Theme> {
        if key == "Default Dark" {
            Some(Self::dark_theme())
        } else if key == "Default Light" {
            Some(Self::light_theme())
        } else {
            None
        }
    }

    pub fn light_theme() -> Theme {
        let mut colors = HashMap::new();

        colors.insert("editor.background".to_string(), "#f5f5f5".to_string());
        colors.insert(
            "editor.gutter.background".to_string(),
            "#f5f5f5".to_string(),
        );
        colors.insert(
            "editor.gutter.foreground".to_string(),
            "#9c9c9c".to_string(),
        );
        colors.insert(
            "editor.gutter.foreground.active".to_string(),
            "#555555".to_string(),
        );
        colors.insert("editor.foreground".to_string(), "#2d2d2d".to_string());
        colors.insert("editor.highlight".to_string(), "#19000000".to_string());

        colors.insert("tab_bar.background".to_string(), "#f7f7f7".to_string());

        colors.insert(
            "file_explorer.background".to_string(),
            "#f5f5f5".to_string(),
        );
        colors.insert(
            "command_palette.background".to_string(),
            "#f5f5f5".to_string(),
        );
        colors.insert(
            "command_palette.border".to_string(),
            "#26000000".to_string(),
        );
        colors.insert(
            "command_palette.shadow".to_string(),
            "#33000000".to_string(),
        );

        colors.insert("sidebar.background".to_string(), "#f5f5f5".to_string());
        colors.insert("ui.foreground".to_string(), "#2d2d2d".to_string());
        colors.insert("ui.foreground.muted".to_string(), "#828282".to_string());
        colors.insert(
            "ui.foreground.very_muted".to_string(),
            "#9a9a9a".to_string(),
        );
        colors.insert("ui.background".to_string(), "#f5f5f5".to_string());
        colors.insert("ui.background.hover".to_string(), "#ededed".to_string());
        colors.insert("ui.accent".to_string(), "#a589d1".to_string());
        colors.insert("ui.accent.hover".to_string(), "#bca3e0".to_string());
        colors.insert("ui.accent.pressed".to_string(), "#8e72b8".to_string());
        colors.insert("ui.accent.muted".to_string(), "#4d3e66".to_string());
        colors.insert("ui.border".to_string(), "#d3d3d3".to_string());
        colors.insert(
            "titlebar.button.foreground".to_string(),
            "#5a5a5a".to_string(),
        );
        colors.insert("titlebar.button.hover".to_string(), "#e6e6e6".to_string());
        colors.insert("titlebar.button.pressed".to_string(), "#dcdcdc".to_string());
        colors.insert("tab.active".to_string(), "#f5f5f5".to_string());
        colors.insert("tab.inactive".to_string(), "#e1e1e1".to_string());
        colors.insert("tab.hover".to_string(), "#e7e7e7".to_string());

        Self {
            name: "Default Light".to_string(),
            colors,
        }
    }

    pub fn dark_theme() -> Theme {
        let mut colors = HashMap::new();

        colors.insert("editor.background".to_string(), "#000000".to_string());
        colors.insert(
            "editor.gutter.background".to_string(),
            "#000000".to_string(),
        );
        colors.insert(
            "editor.gutter.foreground".to_string(),
            "#505050".to_string(),
        );
        colors.insert(
            "editor.gutter.foreground.active".to_string(),
            "#C8C8C8".to_string(),
        );
        colors.insert("editor.foreground".to_string(), "#d4d4d4".to_string());
        colors.insert("editor.highlight".to_string(), "#19ffffff".to_string());

        colors.insert("tab_bar.background".to_string(), "#000000".to_string());

        colors.insert(
            "file_explorer.background".to_string(),
            "#000000".to_string(),
        );
        colors.insert(
            "command_palette.background".to_string(),
            "#000000".to_string(),
        );
        colors.insert(
            "command_palette.border".to_string(),
            "#40ffffff".to_string(),
        );
        colors.insert(
            "command_palette.shadow".to_string(),
            "#66000000".to_string(),
        );

        colors.insert("sidebar.background".to_string(), "#000000".to_string());
        colors.insert("ui.foreground".to_string(), "#d4d4d4".to_string());
        colors.insert("ui.foreground.muted".to_string(), "#a4a4a4".to_string());
        colors.insert(
            "ui.foreground.very_muted".to_string(),
            "#676767".to_string(),
        );
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

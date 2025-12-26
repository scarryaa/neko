use serde::{Deserialize, Serialize};
use std::collections::HashMap;

pub mod theme_manager;
pub use theme_manager::ThemeManager;

macro_rules! colors {
    ($($key:literal => $value:literal),* $(,)?) => {{
        let mut map = HashMap::new();
        $(
            map.insert($key.to_string(), $value.to_string());
        )*
        map
    }};
}

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
        let colors = colors! {
            // Editor
            "editor.background" => "#f5f5f5",
            "editor.foreground" => "#2d2d2d",
            "editor.highlight"  => "#19000000",

            // Gutter
            "editor.gutter.background"         => "#f5f5f5",
            "editor.gutter.foreground"         => "#9c9c9c",
            "editor.gutter.foreground.active"  => "#555555",

            // Tab Bar
            "tab_bar.background" => "#f7f7f7",

            // Tabs
            "tab.active"   => "#f5f5f5",
            "tab.inactive" => "#e1e1e1",
            "tab.hover"    => "#e7e7e7",

            // Sidebar / File Explorer
            "sidebar.background"        => "#f5f5f5",
            "file_explorer.background"  => "#f5f5f5",

            // Command Palette
            "command_palette.background" => "#f5f5f5",
            "command_palette.border"     => "#26000000",
            "command_palette.shadow"     => "#33000000",

            // Context Menu
            "context_menu.background"          => "#f5f5f5",
            "context_menu.border"              => "#d3d3d3",
            "context_menu.foreground"          => "#2d2d2d",
            "context_menu.foreground.disabled" => "#9a9a9a",
            "context_menu.label"               => "#2d2d2d",
            "context_menu.label.disabled"      => "#9a9a9a",
            "context_menu.shortcut"            => "#5a5a5a",
            "context_menu.shortcut.disabled"   => "#b0b0b0",
            "context_menu.button.hover"        => "#dadada",
            "context_menu.shadow"              => "#33000000",

            // Empty State New Tab Button
            "empty_state.button.background"        => "#a589d1",
            "empty_state.button.background.hover"  => "#bca3e0",
            "empty_state.button.foreground"        => "#f5f5f5",
            "empty_state.button.border"            => "#a589d1",

            // UI
            "ui.background"          => "#f5f5f5",
            "ui.background.hover"    => "#ededed",
            "ui.border"              => "#d3d3d3",
            "ui.foreground"          => "#2d2d2d",
            "ui.foreground.muted"    => "#828282",
            "ui.foreground.very_muted" => "#9a9a9a",
            "ui.foreground.inverted" => "#2d2d2d",

            // Accent
            "ui.accent"              => "#a589d1",
            "ui.accent.hover"        => "#bca3e0",
            "ui.accent.pressed"      => "#8e72b8",
            "ui.accent.muted"        => "#4d3e66",
            "ui.accent.foreground"  => "#f5f5f5",

            // Scrollbar
            "ui.scrollbar.thumb"       => "#aaaaaa",
            "ui.scrollbar.thumb.hover" => "#cccccc",

            // Titlebar
            "titlebar.button.foreground" => "#5a5a5a",
            "titlebar.button.hover"      => "#e6e6e6",
            "titlebar.button.pressed"    => "#dcdcdc",
        };

        Theme {
            name: "Default Light".to_string(),
            colors,
        }
    }

    pub fn dark_theme() -> Theme {
        let colors = colors! {
            // Editor
            "editor.background" => "#000000",
            "editor.foreground" => "#d4d4d4",
            "editor.highlight"  => "#19ffffff",

            // Gutter
            "editor.gutter.background"        => "#000000",
            "editor.gutter.foreground"        => "#505050",
            "editor.gutter.foreground.active" => "#c8c8c8",

            // Tab Bar
            "tab_bar.background" => "#000000",

            // Tabs
            "tab.active"   => "#000000",
            "tab.inactive" => "#202020",
            "tab.hover"    => "#101010",

            // Sidebar / File Explorer
            "sidebar.background"       => "#000000",
            "file_explorer.background" => "#000000",

            // Command Palette
            "command_palette.background" => "#000000",
            "command_palette.border"     => "#40ffffff",
            "command_palette.shadow"     => "#66000000",

            // Context Menu
            "context_menu.background"          => "#000000",
            "context_menu.border"              => "#3c3c3c",
            "context_menu.foreground"          => "#d4d4d4",
            "context_menu.foreground.disabled" => "#676767",
            "context_menu.label"               => "#d4d4d4",
            "context_menu.label.disabled"      => "#676767",
            "context_menu.shortcut"            => "#a4a4a4",
            "context_menu.shortcut.disabled"   => "#4f4f4f",
            "context_menu.button.hover"        => "#2c2c2c",
            "context_menu.shadow"              => "#66000000",

            // Empty State New Tab Button
            "empty_state.button.background"        => "#a589d1",
            "empty_state.button.background.hover"  => "#bca3e0",
            "empty_state.button.foreground"        => "#f5f5f5",
            "empty_state.button.border"            => "#a589d1",

            // UI
            "ui.background"           => "#000000",
            "ui.background.hover"     => "#2c2c2c",
            "ui.border"               => "#3c3c3c",
            "ui.foreground"           => "#d4d4d4",
            "ui.foreground.muted"     => "#a4a4a4",
            "ui.foreground.very_muted"=> "#676767",
            "ui.foreground.inverted"  => "#2d2d2d",

            // Accent
            "ui.accent"               => "#a589d1",
            "ui.accent.hover"         => "#bca3e0",
            "ui.accent.pressed"       => "#8e72b8",
            "ui.accent.muted"         => "#4d3e66",
            "ui.accent.foreground"   => "#f5f5f5",

            // Scrollbar
            "ui.scrollbar.thumb"       => "#555555",
            "ui.scrollbar.thumb.hover" => "#666666",

            // Titlebar
            "titlebar.button.foreground" => "#a0a0a0",
            "titlebar.button.hover"      => "#131313",
            "titlebar.button.pressed"    => "#222222",
        };

        Theme {
            name: "Default Dark".to_string(),
            colors,
        }
    }
}

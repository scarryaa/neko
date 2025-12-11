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
        colors.insert("interface.background".to_string(), "#000000".to_string());
        colors.insert("interface.border".to_string(), "#3c3c3c".to_string());

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

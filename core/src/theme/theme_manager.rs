use std::collections::HashMap;

use super::Theme;

pub struct ThemeManager {
    current_theme: Theme,
    loaded_themes: HashMap<String, Theme>,
}

impl Default for ThemeManager {
    fn default() -> Self {
        Self::new()
    }
}

impl ThemeManager {
    pub fn new() -> Self {
        let default_theme = Theme::default();
        let light_theme = Theme::light_theme();
        let mut themes = HashMap::new();

        themes.insert(default_theme.name.clone(), default_theme.clone());
        themes.insert(light_theme.name.clone(), light_theme);

        Self {
            current_theme: default_theme,
            loaded_themes: themes,
        }
    }

    pub fn register_theme(&mut self, theme: Theme) {
        self.loaded_themes.insert(theme.name.clone(), theme);
    }

    pub fn set_theme(&mut self, theme_name: &str) -> bool {
        if let Some(theme) = self.loaded_themes.get(theme_name) {
            self.current_theme = theme.clone();
            return true;
        }

        false
    }

    pub fn get_color(&self, key: &str) -> &str {
        self.current_theme.get_color(key)
    }

    pub fn get_current_theme_name(&self) -> &str {
        &self.current_theme.name
    }
}

use crate::ThemeManager;

pub(crate) fn new_theme_manager() -> std::io::Result<Box<ThemeManager>> {
    let theme_manager = ThemeManager::new();

    Ok(Box::new(theme_manager))
}

impl ThemeManager {
    pub(crate) fn get_color_wrapper(&mut self, key: &str) -> String {
        self.get_color(key).to_string()
    }

    pub(crate) fn get_current_theme_name_wrapper(&self) -> String {
        self.get_current_theme_name().to_string()
    }
}

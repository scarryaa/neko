use std::{io, path::Path};

pub struct DocumentManager;

impl DocumentManager {
    pub fn save_to_path(path: &Path, content: &str) -> io::Result<()> {
        std::fs::write(path, content)
    }

    pub fn load_from_path(path: &Path) -> io::Result<String> {
        std::fs::read_to_string(path)
    }
}

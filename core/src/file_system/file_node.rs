use std::{ffi::OsStr, fs::DirEntry};

#[derive(Clone, Debug)]
pub struct FileNode {
    pub path: String,
    pub name: String,
    pub is_dir: bool,
    pub is_hidden: bool,
    pub size: u64,
    pub modified: u64,
    pub depth: usize,
}

impl FileNode {
    pub fn from_entry(entry: DirEntry) -> Option<Self> {
        let path = entry.path();
        let metadata = entry.metadata().ok()?;
        let file_name = path.file_name()?.to_string_lossy().into_owned();

        Some(FileNode {
            path: path.to_string_lossy().into_owned(),
            name: path.file_name()?.to_string_lossy().into_owned(),
            is_dir: metadata.is_dir(),
            is_hidden: file_name.starts_with('.')
                || path.ancestors().any(|a| {
                    a.file_name()
                        .unwrap_or(OsStr::new(""))
                        .to_str()
                        .unwrap()
                        .starts_with(".")
                }),
            size: metadata.len(),
            modified: metadata
                .modified()
                .ok()?
                .duration_since(std::time::UNIX_EPOCH)
                .ok()?
                .as_secs(),
            depth: 0,
        })
    }

    pub fn path_str(&self) -> String {
        self.path.clone()
    }

    pub fn name_str(&self) -> String {
        self.name.clone()
    }
}

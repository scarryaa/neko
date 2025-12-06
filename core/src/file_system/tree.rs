use std::{
    ffi::{CString, c_char},
    fs, io,
    path::PathBuf,
};

#[repr(C)]
pub struct FileNode {
    pub path: *const c_char,
    pub name: *const c_char,
    pub is_dir: bool,
    pub is_hidden: bool,
    pub size: u64,
    pub modified: u64,
}

impl Drop for FileNode {
    fn drop(&mut self) {
        unsafe {
            if !self.path.is_null() {
                let _ = CString::from_raw(self.path as *mut c_char);
            }

            if !self.name.is_null() {
                let _ = CString::from_raw(self.name as *mut c_char);
            }
        }
    }
}

#[repr(C)]
pub struct FileTree {
    nodes: Vec<FileNode>,
    root_path: PathBuf,
}

impl FileTree {
    pub fn new(root: &str) -> Result<Self, io::Error> {
        let entries = fs::read_dir(root)?;

        let nodes: Vec<FileNode> = entries
            .filter_map(|entry| {
                let entry = entry.ok()?;
                let path = entry.path();
                let metadata = entry.metadata().ok()?;

                Some(FileNode {
                    path: CString::new(path.to_string_lossy().as_ref())
                        .ok()?
                        .into_raw(),
                    name: CString::new(path.file_name()?.to_string_lossy().as_ref())
                        .ok()?
                        .into_raw(),
                    is_dir: metadata.is_dir(),
                    is_hidden: path
                        .file_name()
                        .and_then(|n| n.to_str())
                        .map(|n| n.starts_with('.'))
                        .unwrap_or(false),
                    size: metadata.len(),
                    modified: metadata
                        .modified()
                        .ok()?
                        .duration_since(std::time::UNIX_EPOCH)
                        .ok()?
                        .as_secs(),
                })
            })
            .collect();

        Ok(Self {
            nodes,
            root_path: PathBuf::from(root),
        })
    }
}

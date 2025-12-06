use std::{
    collections::HashMap,
    ffi::{CString, c_char},
    fs::{self, DirEntry},
    io,
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

impl FileNode {
    pub fn from_entry(entry: DirEntry) -> Option<Self> {
        let path = entry.path();
        let metadata = entry.metadata().ok()?;
        let file_name = path.file_name()?.to_string_lossy().into_owned();

        Some(FileNode {
            path: CString::new(path.to_string_lossy().as_ref())
                .ok()?
                .into_raw(),
            name: CString::new(path.file_name()?.to_string_lossy().as_ref())
                .ok()?
                .into_raw(),
            is_dir: metadata.is_dir(),
            is_hidden: file_name.starts_with('.'),
            size: metadata.len(),
            modified: metadata
                .modified()
                .ok()?
                .duration_since(std::time::UNIX_EPOCH)
                .ok()?
                .as_secs(),
        })
    }
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
    expanded: HashMap<PathBuf, Vec<FileNode>>,
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
            expanded: HashMap::new(),
        })
    }

    pub fn get_children(&mut self, path: &str) -> &[FileNode] {
        let path_buf = PathBuf::from(path);

        self.expanded.entry(path_buf.clone()).or_insert_with(|| {
            fs::read_dir(path)
                .ok()
                .map(|entries| {
                    entries
                        .filter_map(|entry| {
                            let entry = entry.ok()?;
                            FileNode::from_entry(entry)
                        })
                        .collect()
                })
                .unwrap_or_default()
        })
    }
}

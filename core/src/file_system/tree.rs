use std::{
    collections::HashMap,
    ffi::{CStr, CString, c_char},
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

    pub fn path_str(&self) -> String {
        unsafe { CStr::from_ptr(self.path).to_string_lossy().into_owned() }
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

    pub fn get_visible_nodes(&self) -> Vec<&FileNode> {
        let mut visible = Vec::new();
        self.collect_visible(&self.root_path, &mut visible, 0);
        visible
    }

    pub fn collect_visible<'a>(
        &'a self,
        path: &PathBuf,
        visible: &mut Vec<&'a FileNode>,
        depth: usize,
    ) {
        if let Some(children) = self.expanded.get(path) {
            for node in children {
                visible.push(node);

                let node_path = PathBuf::from(node.path_str());
                if node.is_dir && self.expanded.contains_key(&node_path) {
                    self.collect_visible(&node_path, visible, depth + 1);
                }
            }
        }
    }

    pub fn next(&self, current_path: &str) -> Option<&FileNode> {
        let visible = self.get_visible_nodes();
        let current_idx = visible.iter().position(|n| n.path_str() == current_path)?;
        visible.get(current_idx + 1).copied()
    }

    pub fn prev(&self, current_path: &str) -> Option<&FileNode> {
        let visible = self.get_visible_nodes();
        let current_idx = visible.iter().position(|n| n.path_str() == current_path)?;
        if current_idx > 0 {
            visible.get(current_idx - 1).copied()
        } else {
            None
        }
    }

    pub fn is_expanded(&self, path: &str) -> bool {
        self.expanded.contains_key(&PathBuf::from(path))
    }
}

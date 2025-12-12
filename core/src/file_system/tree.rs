use std::{
    collections::{HashMap, HashSet},
    ffi::{CStr, CString, c_char},
    fs::{self, DirEntry},
    io,
    path::{Path, PathBuf},
};

#[derive(Clone, Debug)]
pub struct FileNode {
    pub path: *const c_char,
    pub name: *const c_char,
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
            depth: 0,
        })
    }

    pub fn path_str(&self) -> String {
        unsafe { CStr::from_ptr(self.path).to_string_lossy().into_owned() }
    }

    pub fn name_str(&self) -> String {
        unsafe { CStr::from_ptr(self.name).to_string_lossy().into_owned() }
    }

    /// Returns the free strings of this [`FileNode`].
    ///
    /// # Safety
    ///
    /// Call this manually when C side is done with the strings
    pub unsafe fn free_strings(&mut self) {
        if !self.path.is_null() {
            let _ = unsafe { CString::from_raw(self.path as *mut c_char) };
            self.path = std::ptr::null();
        }
        if !self.name.is_null() {
            let _ = unsafe { CString::from_raw(self.name as *mut c_char) };
            self.name = std::ptr::null();
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct FileTree {
    pub nodes: Vec<FileNode>,
    root_path: PathBuf,
    expanded: HashMap<PathBuf, Vec<FileNode>>,
    selected: HashSet<PathBuf>,
    current: Option<PathBuf>,
    pub cached_visible: Vec<FileNode>,
}

impl FileTree {
    pub fn new(root: Option<&str>) -> Result<Self, io::Error> {
        if root.is_none() || root.is_some_and(|r| r.is_empty()) {
            return Ok(Self {
                nodes: Vec::new(),
                root_path: PathBuf::new(),
                expanded: HashMap::new(),
                selected: HashSet::new(),
                current: None,
                cached_visible: Vec::new(),
            });
        }

        let entries = fs::read_dir(root.unwrap_or(""))?;

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
                    depth: 1,
                })
            })
            .collect();

        Ok(Self {
            nodes,
            root_path: PathBuf::from(root.unwrap_or("")),
            expanded: HashMap::new(),
            selected: HashSet::new(),
            current: None,
            cached_visible: Vec::new(),
        })
    }

    pub fn set_root_path(&mut self, path: &str) {
        self.root_path = path.into();
    }

    pub fn get_children(&mut self, path: &str) -> &[FileNode] {
        let path_buf = PathBuf::from(path);

        self.expanded.entry(path_buf.clone()).or_insert_with(|| {
            let mut children: Vec<FileNode> = fs::read_dir(path)
                .ok()
                .map(|entries| {
                    entries
                        .filter_map(|entry| {
                            let entry = entry.ok()?;
                            FileNode::from_entry(entry)
                        })
                        .collect()
                })
                .unwrap_or_default();

            children.sort_by(|a, b| match (a.is_dir, b.is_dir) {
                (true, false) => std::cmp::Ordering::Less,
                (false, true) => std::cmp::Ordering::Greater,
                _ => a
                    .name_str()
                    .to_lowercase()
                    .cmp(&b.name_str().to_lowercase()),
            });

            children
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

    pub fn collect_visible_owned(&self, path: &PathBuf, visible: &mut Vec<FileNode>, depth: usize) {
        if let Some(children) = self.expanded.get(path) {
            for node in children {
                let path_string = node.path_str().to_string();
                let name_string = node.name_str().to_string();

                let node_with_depth = FileNode {
                    path: CString::new(path_string).unwrap().into_raw(),
                    name: CString::new(name_string).unwrap().into_raw(),
                    is_dir: node.is_dir,
                    is_hidden: node.is_hidden,
                    size: node.size,
                    modified: node.modified,
                    depth,
                };

                visible.push(node_with_depth);

                let node_path = PathBuf::from(node.path_str());
                if node.is_dir && self.expanded.contains_key(&node_path) {
                    self.collect_visible_owned(&node_path, visible, depth + 1);
                }
            }
        }
    }

    pub fn get_visible_nodes_owned(&self) -> Vec<FileNode> {
        let mut visible = Vec::new();
        self.collect_visible_owned(&self.root_path, &mut visible, 0);
        visible
    }

    pub fn get_node(&self, current_path: &str) -> Option<&FileNode> {
        let visible = self.get_visible_nodes();
        let current_idx = visible.iter().position(|n| n.path_str() == current_path)?;
        visible.get(current_idx).copied()
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

    pub fn toggle_expanded(&mut self, path: &str) {
        let path_buf = PathBuf::from(path);

        if !path_buf.is_dir() {
            return;
        }

        if self.is_expanded(path) {
            self.expanded.remove(&path_buf);
        } else {
            self.get_children(path);
        }
    }

    pub fn set_collapsed(&mut self, path: &str) {
        let path_buf = PathBuf::from(path);

        if path_buf.is_dir() && self.is_expanded(path) {
            self.expanded.remove(&path_buf);
        }
    }

    pub fn set_expanded(&mut self, path: &str) {
        let path_buf = PathBuf::from(path);

        if path_buf.is_dir() && !self.is_expanded(path) {
            self.get_children(path);
        }
    }

    pub fn is_expanded(&self, path: &str) -> bool {
        self.expanded.contains_key(&PathBuf::from(path))
    }

    pub fn select(&mut self, path: &str) {
        self.selected.insert(PathBuf::from(path));
    }

    pub fn unselect(&mut self, path: &str) {
        self.selected.remove(&PathBuf::from(path));
    }

    pub fn toggle_select(&mut self, path: &str) {
        let path_buf = PathBuf::from(path);

        if self.selected.contains(&path_buf) {
            self.selected.remove(&path_buf);
        } else {
            self.selected.insert(path_buf);
        }
    }

    pub fn set_current(&mut self, path: &str) {
        self.current = Some(PathBuf::from(path));
    }

    pub fn clear_current(&mut self) {
        self.current = None;
    }

    pub fn is_selected(&self, path: &str) -> bool {
        self.selected.contains(&PathBuf::from(path))
    }

    pub fn get_selected(&self) -> Vec<&PathBuf> {
        self.selected.iter().collect()
    }

    pub fn is_current(&self, path: &str) -> bool {
        self.current.as_deref() == Some(Path::new(path))
    }

    pub fn get_current(&self) -> Option<PathBuf> {
        self.current.clone()
    }

    pub fn clear_selection(&mut self) {
        self.selected.clear();
    }

    pub fn get_index(&self) -> usize {
        let visible = self.get_visible_nodes();
        visible
            .iter()
            .position(|n| n.path_str() == self.current.clone().unwrap().to_str().unwrap())
            .unwrap()
    }
}

impl Drop for FileTree {
    fn drop(&mut self) {
        // Free all cached visible strings
        for mut node in std::mem::take(&mut self.cached_visible) {
            unsafe {
                node.free_strings();
            }
        }

        // Free strings in expanded nodes
        for (_, nodes) in std::mem::take(&mut self.expanded) {
            for mut node in nodes {
                unsafe {
                    node.free_strings();
                }
            }
        }

        // Free strings in root nodes
        for mut node in std::mem::take(&mut self.nodes) {
            unsafe {
                node.free_strings();
            }
        }
    }
}

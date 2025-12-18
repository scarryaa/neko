use std::{
    collections::{HashMap, HashSet},
    ffi::OsStr,
    fs::{self, DirEntry},
    io,
    path::{Path, PathBuf},
};

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
                    path: path.to_string_lossy().into_owned(),
                    name: path.file_name()?.to_string_lossy().into_owned(),
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
        self.root_path = PathBuf::from(path);

        self.expanded.clear();
        self.selected.clear();
        self.current = None;
        self.cached_visible.clear();
        self.nodes.clear();

        if path.is_empty() {
            return;
        }

        self.nodes = fs::read_dir(path)
            .ok()
            .map(|entries| {
                entries
                    .filter_map(|entry| FileNode::from_entry(entry.ok()?))
                    .collect()
            })
            .unwrap_or_default();
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
                _ => a.name.to_lowercase().cmp(&b.name.to_lowercase()),
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
        _depth: usize,
    ) {
        if let Some(children) = self.expanded.get(path) {
            for node in children {
                visible.push(node);

                let node_path = PathBuf::from(node.path_str());
                if node.is_dir && self.expanded.contains_key(&node_path) {
                    self.collect_visible(&node_path, visible, _depth + 1);
                }
            }
        }
    }

    pub fn collect_visible_owned(&self, path: &PathBuf, visible: &mut Vec<FileNode>, depth: usize) {
        if let Some(children) = self.expanded.get(path) {
            for node in children {
                let node_with_depth = FileNode {
                    path: node.path.clone(),
                    name: node.name.clone(),
                    is_dir: node.is_dir,
                    is_hidden: node.is_hidden,
                    size: node.size,
                    modified: node.modified,
                    depth,
                };

                visible.push(node_with_depth);

                let node_path = PathBuf::from(&node.path);
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
        let current_idx = visible.iter().position(|n| n.path == current_path)?;
        visible.get(current_idx).copied()
    }

    pub fn next(&self, current_path: &str) -> Option<&FileNode> {
        let visible = self.get_visible_nodes();
        let current_idx = visible.iter().position(|n| n.path == current_path)?;
        visible.get(current_idx + 1).copied()
    }

    pub fn prev(&self, current_path: &str) -> Option<&FileNode> {
        let visible = self.get_visible_nodes();
        let current_idx = visible.iter().position(|n| n.path == current_path)?;
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

    pub fn refresh_dir(&mut self, path: &str) {
        let path_buf = PathBuf::from(path);

        if path_buf.is_dir() {
            self.expanded.remove(&path_buf);
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
        if self.current.is_none() {
            return 0;
        }

        let visible = self.get_visible_nodes();
        visible
            .iter()
            .position(|n| n.path == self.current.clone().unwrap().to_str().unwrap())
            .unwrap()
    }
}

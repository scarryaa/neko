use crate::{FileIoManager, FileNode, FileSystemResult};
use std::{
    collections::{HashMap, HashSet},
    path::{Path, PathBuf},
};

/// Represents a file tree.
///
/// Stores relevant information about the files/directories therein, such as the root directory
/// path, the nodes (and their children) contained within the root path, and other useful metadata
/// like which directories are 'expanded' or 'selected'.
#[derive(Debug)]
pub struct FileTree {
    /// The root path of the tree; e.g. the top-most directory.
    pub root_path: Option<PathBuf>,
    /// Stores all of the tree's [`FileNode`]s, keyed by the node's path.
    pub loaded_nodes: HashMap<PathBuf, Vec<FileNode>>,
    /// Holds the set of currently expanded paths, where each path is from a [`FileNode`]
    /// entry (must be a directory).
    expanded_paths: HashSet<PathBuf>,
    /// Holds the set of currently selected paths, where each path is from a [`FileNode`]
    /// entry.
    selected_paths: HashSet<PathBuf>,
    /// The path currently used as the anchor for navigation or other movement actions.
    current_path: Option<PathBuf>,
}

impl FileTree {
    /// Constructs a `FileTree` for the given root directory.
    ///
    /// If `Some(path)` is provided and the path is a valid root path, the
    /// immediate children of that directory are loaded and stored in
    /// `loaded_nodes`, and the root directory is marked as expanded.
    ///
    /// If `None` is provided, or if the path is not a root path, an empty
    /// `FileTree` is returned (see [`FileTree::empty`]).
    pub fn new<P: AsRef<Path>>(root_path_opt: Option<P>) -> FileSystemResult<Self> {
        // No root path was provided, so initialize with an "empty" state.
        let root_path_buf = match root_path_opt {
            None => return Ok(Self::empty()),
            Some(path) => {
                let path_ref = path.as_ref();
                if !path_ref.has_root() {
                    return Ok(Self::empty());
                }
                path_ref.to_path_buf()
            }
        };

        let mut loaded_nodes = HashMap::new();
        Self::get_children_impl(&mut loaded_nodes, &root_path_buf)?;

        let mut expanded_paths = HashSet::new();
        expanded_paths.insert(root_path_buf.clone());

        Ok(Self {
            root_path: Some(root_path_buf),
            loaded_nodes,
            expanded_paths,
            selected_paths: HashSet::new(),
            current_path: None,
        })
    }

    /// Creates an empty `FileTree` with no root path and no loaded nodes.
    ///
    /// This represents a valid tree in an "uninitialized" state; no files are
    /// loaded, no directories are expanded, and there is no current or
    /// selected path.
    pub fn empty() -> Self {
        Self {
            root_path: None,
            loaded_nodes: HashMap::new(),
            expanded_paths: HashSet::new(),
            selected_paths: HashSet::new(),
            current_path: None,
        }
    }

    pub fn set_root_path<P: AsRef<Path>>(&mut self, root_path: P) -> FileSystemResult<()> {
        let path = root_path.as_ref().to_path_buf();
        let new_tree = FileTree::new(Some(path))?;

        *self = new_tree;

        Ok(())
    }

    pub fn get_children<P: AsRef<Path>>(
        &mut self,
        directory_path: P,
    ) -> FileSystemResult<&[FileNode]> {
        Self::get_children_impl(&mut self.loaded_nodes, directory_path)
    }

    pub fn get_children_impl<P: AsRef<Path>>(
        loaded_nodes: &mut HashMap<PathBuf, Vec<FileNode>>,
        directory_path: P,
    ) -> FileSystemResult<&[FileNode]> {
        let path = directory_path.as_ref().to_path_buf();

        // If the nodes for the given path aren't already loaded, add them
        if !loaded_nodes.contains_key(&path) {
            let mut children = Vec::new();
            for entry_res in FileIoManager::read_dir(&path)? {
                let entry = entry_res?;
                // Actual depth will be updated later
                let node = FileNode::from_entry(entry, 1)?;
                children.push(node);
            }

            children.sort_by_key(|n| (!n.is_dir, n.name.to_lowercase()));
            loaded_nodes.insert(path.clone(), children);
        }

        // Otherwise just return the already-loaded nodes
        Ok(loaded_nodes.get(&path).unwrap())
    }

    /// Marks the provided path as collapsed, then fetches its children., serving as a "refresh" for
    /// that path.
    ///
    /// Mostly used for scoped updates after, e.g. creating a new file/folder in a directory.
    pub fn refresh_dir<P: AsRef<Path>>(&mut self, path: P) -> FileSystemResult<()> {
        let path_buf = path.as_ref().to_path_buf();

        if path_buf.is_dir() {
            self.expanded_paths.remove(&path_buf);
            self.get_children(path)?;
        }

        Ok(())
    }

    /// Returns true if the provided path is expanded, and false if it is not.
    fn is_expanded<P: AsRef<Path>>(&self, path: P) -> bool {
        self.expanded_paths.contains(path.as_ref())
    }

    /// Returns the "next" node in the tree (the one below the `current_path` node), if any.
    pub fn next<P: AsRef<Path>>(&self, current_path: P) -> Option<FileNode> {
        let path = current_path.as_ref();

        let visible_nodes = self.visible_nodes();
        let idx = visible_nodes.iter().position(|node| node.path == path)?;
        visible_nodes.get(idx + 1).cloned()
    }

    /// Returns the "previous" node in the tree (the one above the `current_path` node), if any.
    pub fn prev<P: AsRef<Path>>(&self, current_path: P) -> Option<FileNode> {
        let path = current_path.as_ref();

        let visible_nodes = self.visible_nodes();
        let idx = visible_nodes.iter().position(|node| node.path == path)?;
        if idx == 0 {
            None
        } else {
            visible_nodes.get(idx - 1).cloned()
        }
    }

    /// Toggles selection on a node, selecting it if it is not selected, and unselecting it if it
    /// is selected.
    pub fn toggle_select_for_path<P: AsRef<Path>>(&mut self, path: P) {
        let path_buf = path.as_ref().to_path_buf();

        match self.selected_paths.contains(&path_buf) {
            true => self.selected_paths.remove(&path_buf),
            false => self.selected_paths.insert(path_buf),
        };
    }

    /// Returns the currently selected path if there is one, or `None`.
    pub fn current_path(&self) -> Option<PathBuf> {
        self.current_path.clone()
    }

    /// Returns the expanded paths by moving it.
    pub fn expanded_owned(&self) -> HashSet<PathBuf> {
        self.expanded_paths.clone()
    }

    /// Returns the selected paths by moving it.
    pub fn selected_owned(&self) -> HashSet<PathBuf> {
        self.selected_paths.clone()
    }

    /// Toggles the expanded/collapsed state of the provided path. If it is not expanded, it is
    /// added to `expanded_paths` and its children are retrieved.
    pub fn toggle_expanded<P: AsRef<Path>>(&mut self, path: P) {
        let path_buf = path.as_ref().to_path_buf();

        if self.is_expanded(&path_buf) {
            self.set_collapsed(&path_buf);
        } else {
            self.set_expanded(&path_buf);
        }
    }

    /// Removes the provided path from `expanded_paths`, effectively marking it as collapsed.
    pub fn set_collapsed<P: AsRef<Path>>(&mut self, path: P) {
        let path_buf = path.as_ref().to_path_buf();

        if path_buf.is_dir() && self.is_expanded(path) {
            self.expanded_paths.remove(&path_buf);
        }
    }

    /// Marks the provided path as expanded and loads its children.
    pub fn set_expanded<P: AsRef<Path>>(&mut self, path: P) {
        let path_buf = path.as_ref().to_path_buf();

        // Only directories can be expanded.
        if !path_buf.is_dir() {
            return;
        }

        // We only need to load the children if we are changing state from collapsed -> expanded.
        if self.expanded_paths.insert(path_buf.clone()) {
            let _ = self.get_children(&path_buf);
        }
    }

    /// Ensures that the directory containing `path` (and all of its ancestors) are marked as
    /// expanded and have their children loaded.
    pub fn ensure_path_visible<P: AsRef<Path>>(&mut self, path: P) -> FileSystemResult<()> {
        let mut path_buf = path.as_ref().to_path_buf();

        // If the provided path is not a directory, "redirect" it to the parent directory.
        if !path_buf.is_dir() {
            if let Some(parent) = path_buf.parent() {
                path_buf = parent.to_path_buf();
            } else {
                // Path has no parent; nothing to expand.
                return Ok(());
            }
        }

        // Return early if the root path is `None`.
        let root_path = match &self.root_path {
            Some(path) => path,
            None => return Ok(()),
        };

        // Return early if the provided path is not a descendant of the root directory/path.
        if !path_buf.starts_with(root_path) {
            return Ok(());
        }

        // Collect all ancestors from `path_buf` up to `root`.
        let mut ancestor_paths = Vec::new();
        let mut current_path = path_buf;

        loop {
            ancestor_paths.push(current_path.clone());

            // Exit if we reached the root path.
            if current_path == *root_path {
                break;
            }

            // Exit if the current path has no parent.
            let Some(parent) = current_path.parent() else {
                break;
            };

            current_path = parent.to_path_buf();
        }

        // Ensure we expand from the root downwards.
        ancestor_paths.reverse();

        for ancestor_path in ancestor_paths {
            self.set_expanded(&ancestor_path);
        }

        Ok(())
    }

    /// Returns a flattened list of all visible nodes in the tree, in sorted
    /// order.
    pub fn visible_nodes(&self) -> Vec<FileNode> {
        let mut visible_nodes = Vec::new();
        let root_path = match &self.root_path {
            Some(path) => path,
            None => return visible_nodes,
        };

        self.collect_visible_owned(root_path, 0, &mut visible_nodes);
        visible_nodes
    }

    fn collect_visible_owned<P: AsRef<Path>>(
        &self,
        directory_path: P,
        depth: usize,
        collected_nodes: &mut Vec<FileNode>,
    ) {
        let path = directory_path.as_ref();
        // If no children have been loaded for this directory, return.
        let Some(children) = self.loaded_nodes.get(path) else {
            return;
        };

        for node in children {
            collected_nodes.push(node.with_depth(depth));

            // Recurse into expanded directories.
            if node.is_dir && self.expanded_paths.contains(&node.path) {
                self.collect_visible_owned(&node.path, depth + 1, collected_nodes);
            }
        }
    }

    /// Sets the `current_path` from the path provided.
    pub fn set_current_path<P: AsRef<Path>>(&mut self, path: P) {
        self.current_path = Some(path.as_ref().to_path_buf());
    }

    /// Clears the `current_path`, setting it to `None`.
    pub fn clear_current_path(&mut self) {
        self.current_path = None;
    }

    /// Clears the current selected node(s).
    pub fn clear_selected_paths(&mut self) {
        self.selected_paths.clear();
    }
}

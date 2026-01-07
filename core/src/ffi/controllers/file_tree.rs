use crate::{
    AppState, FileNode, FileTree,
    ffi::{FileNodeSnapshot, FileSystemErrorFfi, FileTreeSnapshot, MaybeFileNodeSnapshot},
};
use std::{cell::RefCell, collections::HashSet, path::PathBuf, rc::Rc};

pub struct FileTreeController {
    pub(crate) app_state: Rc<RefCell<AppState>>,
}

impl FileTreeController {
    fn access_mut<F, R>(&mut self, f: F) -> R
    where
        F: FnOnce(&mut FileTree) -> R,
    {
        let mut app_state = self.app_state.borrow_mut();
        let file_tree = app_state.get_file_tree_mut();
        f(file_tree)
    }

    fn access<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&FileTree) -> R,
    {
        let app_state = self.app_state.borrow();
        let file_tree = app_state.get_file_tree();
        f(file_tree)
    }

    pub fn set_root_path(
        self: &mut FileTreeController,
        path: &str,
    ) -> Result<(), FileSystemErrorFfi> {
        self.access_mut(|tree| tree.set_root_path(path))
            .map_err(FileSystemErrorFfi::from)
    }

    pub fn get_root_path(&self) -> String {
        self.access(|tree| {
            if let Some(root_path) = &tree.root_path {
                root_path.to_str().unwrap_or("").to_string()
            } else {
                "".to_string()
            }
        })
    }

    pub fn ensure_path_visible(
        self: &mut FileTreeController,
        path: &str,
    ) -> Result<(), FileSystemErrorFfi> {
        self.access_mut(|tree| tree.ensure_path_visible(path))
            .map_err(FileSystemErrorFfi::from)
    }

    pub fn set_expanded(self: &mut FileTreeController, path: &str) {
        self.access_mut(|tree| tree.set_expanded(path))
    }

    pub fn set_collapsed(self: &mut FileTreeController, path: &str) {
        self.access_mut(|tree| tree.set_collapsed(path))
    }

    pub fn toggle_select_for_path(self: &mut FileTreeController, path: &str) {
        self.access_mut(|tree| tree.toggle_select_for_path(path))
    }

    pub fn set_current_path(self: &mut FileTreeController, path: &str) {
        self.access_mut(|tree| tree.set_current_path(path))
    }

    pub fn clear_current_path(self: &mut FileTreeController) {
        self.access_mut(|tree| tree.clear_current_path())
    }

    pub fn refresh_dir(
        self: &mut FileTreeController,
        path: &str,
    ) -> Result<(), FileSystemErrorFfi> {
        self.access_mut(|tree| tree.refresh_dir(path))
            .map_err(FileSystemErrorFfi::from)
    }

    pub fn toggle_expanded(&mut self, path: &str) {
        self.app_state
            .borrow_mut()
            .get_file_tree_mut()
            .toggle_expanded(path);
    }

    pub fn get_next_node(
        &self,
        current_path: &str,
    ) -> Result<FileNodeSnapshot, FileSystemErrorFfi> {
        self.access(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let node = tree.next(current_path).unwrap();

            Ok(Self::make_file_node_snapshot(
                &node,
                &selected_paths,
                &expanded_paths,
                Some(&PathBuf::from(current_path)),
            ))
        })
    }

    pub fn get_prev_node(
        &self,
        current_path: &str,
    ) -> Result<FileNodeSnapshot, FileSystemErrorFfi> {
        self.access(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let node = tree.prev(current_path).unwrap();

            Ok(Self::make_file_node_snapshot(
                &node,
                &selected_paths,
                &expanded_paths,
                Some(&PathBuf::from(current_path)),
            ))
        })
    }

    pub fn get_path_of_parent(&self, path: &str) -> String {
        let path_buf = PathBuf::from(path);
        path_buf
            .parent()
            .unwrap()
            .to_string_lossy()
            .as_ref()
            .to_string()
    }

    pub fn get_children(
        &mut self,
        path: &str,
    ) -> Result<Vec<FileNodeSnapshot>, FileSystemErrorFfi> {
        self.access_mut(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let current_path = tree.current_path();
            let children = tree.get_children(path).map_err(FileSystemErrorFfi::from)?;

            Ok(children
                .iter()
                .map(|node| {
                    Self::make_file_node_snapshot(
                        node,
                        &selected_paths,
                        &expanded_paths,
                        current_path.as_ref(),
                    )
                })
                .collect())
        })
    }

    fn make_file_node_snapshot(
        node: &FileNode,
        selected_paths: &HashSet<PathBuf>,
        expanded_paths: &HashSet<PathBuf>,
        current_path: Option<&PathBuf>,
    ) -> FileNodeSnapshot {
        FileNodeSnapshot {
            path: node.path.clone().to_string_lossy().to_string(),
            name: node.name.clone(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            is_expanded: node.is_dir && expanded_paths.contains(&node.path),
            is_selected: selected_paths.contains(&node.path),
            is_current: current_path == Some(&node.path),
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
    }

    pub fn get_tree_snapshot(&self) -> FileTreeSnapshot {
        self.access(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let current_path = tree.current_path();

            let nodes: Vec<FileNodeSnapshot> = tree
                .visible_nodes()
                .iter()
                .map(|node| {
                    Self::make_file_node_snapshot(
                        node,
                        &selected_paths,
                        &expanded_paths,
                        current_path.as_ref(),
                    )
                })
                .collect();

            let (root_present, root) =
                match tree.root_path.as_ref().is_some_and(|path| path.has_root()) {
                    true => (
                        true,
                        tree.root_path
                            .as_ref()
                            .unwrap()
                            .to_string_lossy()
                            .to_string(),
                    ),
                    false => (false, "".to_string()),
                };

            FileTreeSnapshot {
                root_present,
                root,
                nodes,
            }
        })
    }

    pub fn get_first_node(&self) -> MaybeFileNodeSnapshot {
        self.access(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let current_path = tree.current_path();

            if let Some(first_node) = tree.visible_nodes().first() {
                MaybeFileNodeSnapshot {
                    found_node: true,
                    node: Self::make_file_node_snapshot(
                        first_node,
                        &selected_paths,
                        &expanded_paths,
                        current_path.as_ref(),
                    ),
                }
            } else {
                MaybeFileNodeSnapshot {
                    found_node: false,
                    node: FileNodeSnapshot::default(),
                }
            }
        })
    }

    pub fn get_last_node(&self) -> MaybeFileNodeSnapshot {
        self.access(|tree| {
            let selected_paths = tree.selected_owned();
            let expanded_paths = tree.expanded_owned();
            let current_path = tree.current_path();

            if let Some(last_node) = tree.visible_nodes().last() {
                MaybeFileNodeSnapshot {
                    found_node: true,
                    node: Self::make_file_node_snapshot(
                        last_node,
                        &selected_paths,
                        &expanded_paths,
                        current_path.as_ref(),
                    ),
                }
            } else {
                MaybeFileNodeSnapshot {
                    found_node: false,
                    node: FileNodeSnapshot::default(),
                }
            }
        })
    }
}

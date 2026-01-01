use crate::{
    FileNode, FileTree,
    ffi::{FileNodeSnapshot, FileSystemErrorFfi, FileTreeSnapshot},
};
use std::{collections::HashSet, path::PathBuf};

impl FileTree {
    pub(crate) fn get_next_node_wrapper(
        &self,
        current_path: &str,
    ) -> Result<FileNodeSnapshot, FileSystemErrorFfi> {
        let selected_paths = self.selected_owned();
        let expanded_paths = self.expanded_owned();
        let node = self.next(current_path).unwrap();

        Ok(Self::make_file_node_snapshot(
            &node,
            &selected_paths,
            &expanded_paths,
            Some(&PathBuf::from(current_path)),
        ))
    }

    pub(crate) fn get_prev_node_wrapper(
        &self,
        current_path: &str,
    ) -> Result<FileNodeSnapshot, FileSystemErrorFfi> {
        let selected_paths = self.selected_owned();
        let expanded_paths = self.expanded_owned();
        let node = self.prev(current_path).unwrap();

        Ok(Self::make_file_node_snapshot(
            &node,
            &selected_paths,
            &expanded_paths,
            Some(&PathBuf::from(current_path)),
        ))
    }

    pub(crate) fn get_path_of_parent(&self, path: &str) -> String {
        let path_buf = PathBuf::from(path);
        path_buf
            .parent()
            .unwrap()
            .to_string_lossy()
            .as_ref()
            .to_string()
    }

    pub fn get_children_wrapper(
        &mut self,
        path: &str,
    ) -> Result<Vec<FileNodeSnapshot>, FileSystemErrorFfi> {
        let selected_paths = self.selected_owned();
        let expanded_paths = self.expanded_owned();
        let current_path = self.current_path();
        let children = self.get_children(path).map_err(FileSystemErrorFfi::from)?;

        Ok(children
            .iter()
            .map(|node| {
                let snapshot = Self::make_file_node_snapshot(
                    node,
                    &selected_paths,
                    &expanded_paths,
                    current_path.as_ref(),
                );
                FileNodeSnapshot::from(snapshot)
            })
            .collect())
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

    pub(crate) fn get_tree_snapshot(&self) -> FileTreeSnapshot {
        let selected_paths = self.selected_owned();
        let expanded_paths = self.expanded_owned();
        let current_path = self.current_path();

        let nodes: Vec<FileNodeSnapshot> = self
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

        let (root_present, root) = match self.root_path.as_ref().is_some_and(|path| path.has_root())
        {
            true => (
                true,
                self.root_path
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
    }
}

use crate::{
    FileTree,
    ffi::{FileNodeSnapshot, FileTreeSnapshot},
};
use std::path::PathBuf;

impl FileTree {
    pub(crate) fn get_next_node(&self, current_path: &str) -> FileNodeSnapshot {
        let node = self.next(current_path).unwrap();
        self.make_file_node_snapshot(node)
    }

    pub(crate) fn get_prev_node(&self, current_path: &str) -> FileNodeSnapshot {
        let node = self.prev(current_path).unwrap();
        self.make_file_node_snapshot(node)
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

    pub(crate) fn get_children_wrapper(&mut self, path: &str) -> Vec<FileNodeSnapshot> {
        let children_owned = self.get_children(path).to_vec();

        children_owned
            .iter()
            .map(|node| self.make_file_node_snapshot(node))
            .collect()
    }

    fn make_file_node_snapshot(&self, node: &crate::FileNode) -> FileNodeSnapshot {
        let p = PathBuf::from(node.path_str());

        FileNodeSnapshot {
            path: node.path.clone(),
            name: node.name.clone(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            is_expanded: node.is_dir && self.get_expanded_owned().contains_key(&p),
            is_selected: self.get_selected_owned().contains(&p),
            is_current: self.get_current().as_ref() == Some(&p),
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
    }

    pub(crate) fn get_tree_snapshot(&self) -> FileTreeSnapshot {
        let nodes: Vec<FileNodeSnapshot> = self
            .get_visible_nodes_owned()
            .iter()
            .map(|node| self.make_file_node_snapshot(node))
            .collect();

        let root_present = !self.root_path.as_os_str().is_empty();

        FileTreeSnapshot {
            root_present,
            root: self.root_path.to_string_lossy().to_string(),
            nodes,
        }
    }
}

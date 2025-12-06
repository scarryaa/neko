use std::{ffi::c_char, io, path::PathBuf};

#[repr(C)]
pub struct FileNode {
    pub path: *const c_char,
    pub name: *const c_char,
    pub is_dir: bool,
    pub is_hidden: bool,
    pub size: u64,
    pub modified: u64,
}

#[repr(C)]
pub struct FileTree {
    nodes: Vec<FileNode>,
    root_path: PathBuf,
}

impl FileTree {
    pub fn new(root: &str) -> Result<Self, io::Error> {
        Ok(Self {
            nodes: Vec::new(),
            root_path: root.into(),
        })
    }
}

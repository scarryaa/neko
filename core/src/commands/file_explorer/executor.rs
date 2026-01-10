use super::{
    FileExplorerCommand, FileExplorerCommandError, FileExplorerCommandState, FileExplorerContext,
    FileExplorerNavigationDirection,
};
use crate::{
    AppState, FileExplorerCommandResult, FileExplorerUiIntent, FileIoManager,
    FileOperationsManager, FileTree,
};
use std::{io, path::PathBuf};

pub fn file_explorer_command_state(
    app_state: &AppState,
    path: PathBuf,
) -> Result<FileExplorerCommandState, FileExplorerCommandError> {
    let is_directory = path.is_dir();
    let root_path_opt = app_state.get_file_tree().root_path.clone();
    if root_path_opt.is_none() {
        // Can't do anything if there is no root path, so return.
        return Ok(FileExplorerCommandState::default());
    }

    // Whether the context menu was opened on an item or "empty space".
    let target_is_item = path.is_file() || is_directory;

    Ok(FileExplorerCommandState {
        can_make_new_file: true,
        can_make_new_folder: true,
        can_reveal_in_system: true,
        can_open_in_terminal: true,
        can_find_in_folder: is_directory,
        can_cut: true,
        can_copy: true,
        can_duplicate: true,
        can_paste: true,
        can_copy_path: true,
        can_copy_relative_path: true,
        can_show_history: !is_directory,
        can_rename: true,
        can_delete: target_is_item,
        can_expand_item: is_directory,
        can_collapse_item: is_directory,
        can_collapse_all: !target_is_item,
    })
}

pub fn run_file_explorer_command(
    app_state: &mut AppState,
    id: &str,
    ctx: &FileExplorerContext,
    new_or_rename_item_name: Option<String>,
) -> Result<FileExplorerCommandResult, FileExplorerCommandError> {
    use FileExplorerCommand::*;
    use FileExplorerNavigationDirection::*;

    let root_path_opt = app_state.get_file_tree().root_path.clone();
    if root_path_opt.is_none() {
        // Can't do anything if there is no root path, so return.
        return Ok(FileExplorerCommandResult { intents: vec![] });
    }

    // Unwrapping here should be fine due to the check above.
    let root_path = root_path_opt.unwrap();

    let command: FileExplorerCommand = id
        .parse()
        .map_err(|_| FileExplorerCommandError::UnknownCommand(id.to_string()))?;
    let tree = app_state.get_file_tree_mut();

    // Helper to determine where to create new items.
    // If the target is a directory, put it inside. If it's a file, put it in the parent directory.
    //
    // If item_path is empty, treat it as the root directory.
    let target_directory = if ctx.item_path.as_os_str().is_empty() {
        root_path.clone()
    } else if ctx.item_path.is_dir() {
        ctx.item_path.clone()
    } else {
        ctx.item_path
            .parent()
            .map(|p| p.to_path_buf())
            .unwrap_or_else(|| ctx.item_path.clone())
    };

    enum NodePathOptions {
        First,
        Last,
    }

    let ith_node_path = |tree: &mut FileTree, opt: NodePathOptions, path: PathBuf| {
        tree.get_children(path).map(|nodes| {
            let node = match opt {
                NodePathOptions::First => nodes.first(),
                NodePathOptions::Last => nodes.last(),
            };
            node.map(|node| node.path.clone())
        })
    };

    let mut select_first_node_if_current_empty = || {
        if ctx.item_path.as_os_str().is_empty() {
            _ = ith_node_path(tree, NodePathOptions::First, root_path.clone()).map(
                |first_node_path| {
                    first_node_path.map(|first_node_path| {
                        tree.set_current_path(first_node_path);
                    })
                },
            );
        }
    };

    let mut ui_intents: Vec<FileExplorerUiIntent> = Vec::new();

    // NOTE: The `DirectoryRefreshed` UI intent isn't strictly needed in most/all cases since
    // `refresh_dir` reloads the directory items (and the UI side fetches a snapshot of the current
    // state every draw), but is good to have anyway in case we start caching items on the UI side.

    // TODO(scarlet): Implement a more granular refresh for new file/new folder/rename/delete operations?
    // E.g. just add the new item to the tree/update (or remove) the item instead of reloading the whole directory.
    match command {
        // Handles the 'New File' event.
        //
        // Creates a new file with the specified name, but fails if the name is empty or if
        // a file with that name already exists.
        NewFile => {
            let name = PathBuf::from(
                new_or_rename_item_name
                    .expect("New item name is required for new file operation")
                    .clone(),
            );

            let target_path = target_directory.join(name);

            // Only create the file if it doesn't already exist.
            if !FileIoManager::file_exists(&target_path) {
                FileIoManager::write_file(&target_path, "")
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            }

            // Refresh the directory so the new file appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
            tree.set_current_path(target_path);

            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: target_directory,
            });
        }
        // Handles the 'New Folder' event.
        //
        // Creates a new directory with the specified name, but fails if the name is empty or if
        // a directory with that name already exists.
        NewFolder => {
            let name = PathBuf::from(
                new_or_rename_item_name
                    .expect("New item name is required for new folder operation")
                    .clone(),
            );

            let target_path = target_directory.join(name);

            // Only create the directory if it doesn't already exist.
            if !FileIoManager::directory_exists(&target_path) {
                FileIoManager::create_directory(&target_path)
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            }

            // Refresh the directory so the new folder appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
            tree.set_current_path(target_path);

            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: target_directory,
            });
        }
        // Handles the 'Reveal' event.
        //
        // Attempts to show the current node in the OS file explorer.
        Reveal => {
            // Opens the native OS file explorer pointing to the file.
            let path = &ctx.item_path;

            #[cfg(target_os = "macos")]
            let _ = std::process::Command::new("open")
                .arg("-R")
                .arg(path)
                .spawn();

            #[cfg(target_os = "windows")]
            let _ = std::Process::Command::new("explorer")
                .arg("/select,")
                .arg(path)
                .spawn();

            #[cfg(target_os = "linux")]
            {
                // We try to open the parent dir with xdg-open.
                // TODO(scarlet): Expand on this?
                if let Some(parent) = path.parent() {
                    let _ = std::Process::Command::new("xdg-open").arg(parent).spawn();
                }
            }
        }
        OpenInTerminal => {
            // TODO(scarlet): Implement this once terminal is added.
        }
        FindInFolder => {
            // TODO(scarlet): Implement this once search is added.
        }
        // Handles the 'Duplicate' event.
        //
        // Attempts to duplicate the current node (directory or file).
        Duplicate => {
            let destination_path = FileOperationsManager::duplicate(ctx.item_path.clone())
                .map_err(|e| FileExplorerCommandError::IoError(id.to_string(), e))?;

            if let Some(parent_directory_path) = ctx.item_path.parent()
                && !destination_path.to_string_lossy().is_empty()
            {
                tree.refresh_dir(parent_directory_path).ok();

                // Select the new item.
                tree.set_current_path(destination_path);
                ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                    path: PathBuf::from(parent_directory_path),
                });
            }
        }
        Cut => {
            // Implemented by UI.
        }
        Copy => {
            // Implemented by UI.
        }
        // Handles the 'Paste' event.
        //
        // Pastes an item from the clipboard (in this case, receives a list of files to paste from
        // the UI).
        Paste => {
            let is_cut_operation = ctx.paste_info.is_cut_operation;
            let paste_items = ctx.paste_info.items.clone();

            if paste_items.is_empty() {
                // If there are no items to be pasted, return.
                return Err(FileExplorerCommandError::IoError(
                    id.to_string(),
                    io::Error::new(
                        io::ErrorKind::NotFound,
                        "Attempted paste operation but there are no items to paste",
                    ),
                ));
            }

            for item in paste_items {
                // Verify the item exists.
                if !item.path.exists() {
                    return Err(FileExplorerCommandError::IoError(
                        id.to_string(),
                        io::Error::new(
                            io::ErrorKind::NotFound,
                            "An item to be pasted was not found at its associated path",
                        ),
                    ));
                }

                let final_path: PathBuf;

                if let Some(item_file_name) = item.path.clone().file_name() {
                    let destination_path = FileOperationsManager::get_destination_path(
                        item.path.clone(),
                        ctx.item_path.clone(),
                        &item_file_name.to_string_lossy(),
                    )
                    .map_err(|e| FileExplorerCommandError::IoError(id.to_string(), e))?;

                    if item.is_dir {
                        final_path = FileOperationsManager::handle_paste(
                            item.path.clone(),
                            destination_path,
                            is_cut_operation,
                            true,
                        )
                        .map_err(|e| FileExplorerCommandError::IoError(id.to_string(), e))?;
                    } else {
                        final_path = FileOperationsManager::handle_paste(
                            item.path.clone(),
                            destination_path,
                            is_cut_operation,
                            false,
                        )
                        .map_err(|e| FileExplorerCommandError::IoError(id.to_string(), e))?;
                    }
                } else {
                    return Err(FileExplorerCommandError::IoError(
                        id.to_string(),
                        io::Error::new(io::ErrorKind::NotFound, "Unable to get item file name"),
                    ));
                }

                if let Some(parent_dir) = final_path.parent() {
                    tree.refresh_dir(parent_dir).ok();

                    // Make sure the parent directory is expanded.
                    tree.set_expanded(parent_dir);

                    // Select the new item.
                    tree.set_current_path(&final_path);
                    ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                        path: parent_dir.to_path_buf(),
                    });
                }
            }
        }
        CopyPath => {
            // Implemented by UI.
        }
        CopyRelativePath => {
            // Implemented by UI.
        }
        ShowHistory => {
            // TODO(scarlet): Implement this using git history.
        }
        // Handles the 'Rename' event.
        //
        // Attempts to rename an item, but fails if an item with the new name already exists.
        Rename => {
            let new_filename = PathBuf::from(
                &new_or_rename_item_name
                    .expect("Rename item name is required for rename operation"),
            );
            let parent_dir = ctx.item_path.parent().ok_or_else(|| {
                FileExplorerCommandError::UnknownCommand(
                    "Cannot rename the file system root".into(),
                )
            })?;

            let new_path = parent_dir.join(&new_filename);

            // Check for naming collisions.
            if FileIoManager::exists(&new_path).is_ok_and(|result| result) {
                return Err(FileExplorerCommandError::IoError(
                    id.to_string(),
                    io::Error::new(
                        io::ErrorKind::AlreadyExists,
                        "A file with that name already exists",
                    ),
                ));
            }

            // Rename the item.
            FileIoManager::rename(&ctx.item_path, &new_path)
                .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;

            // Refresh the parent directory so the tree reflects the name change.
            tree.refresh_dir(parent_dir).ok();
            tree.set_current_path(new_path);

            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: parent_dir.to_path_buf(),
            });
        }
        // Handles the 'Delete' event.
        //
        // Deletes the specified node, removing it from the file system and the tree.
        Delete => {
            // Get the previous node ahead of time, since it should be removed later.
            let previous_node = tree.prev(ctx.item_path.clone());
            let next_node = tree.next(ctx.item_path.clone());

            // Delete the file or directory, then refresh the parent.
            match ctx.item_path.is_dir() {
                true => FileIoManager::remove_directory_all(&ctx.item_path)
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?,
                false => FileIoManager::remove_file(&ctx.item_path)
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?,
            }

            if let Some(parent) = ctx.item_path.parent() {
                tree.refresh_dir(parent).ok();

                ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                    path: parent.to_path_buf(),
                });
            }

            // Select the previous node.
            //
            // If there is no previous node, select the next node.
            if let Some(previous_node) = previous_node
                && previous_node.path != root_path
            {
                tree.set_current_path(previous_node.path);
            } else if let Some(next_node) = next_node {
                tree.set_current_path(next_node.path);
            }
        }
        // Handles the 'Expand' event.
        //
        // Expands a directory if it is collapsed, and collapses a directory if it is already
        // expanded.
        Expand => match ctx.item_is_directory && ctx.item_is_expanded {
            true => tree.set_collapsed(ctx.item_path.clone()),
            false => tree.set_expanded(ctx.item_path.clone()),
        },
        // Handles the 'Collapse All' event.
        //
        // Collapses all directories in the tree.
        CollapseAll => {
            tree.collapse_all();
            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed { path: root_path });
        }
        // Navigation event handlers.
        Navigation(direction) => {
            select_first_node_if_current_empty();

            match direction {
                // Handles the 'Left' navigation event.
                //
                // If a valid node is selected:
                // - If it's a directory:
                //    - Expanded: Set to collapsed.
                //    - Collapsed: Move to the parent node (directory), select it, and collapse
                //      it.
                Left => {
                    let parent_node = tree.get_parent(&ctx.item_path);

                    // If the current node is expanded, just collapse it.
                    if ctx.item_is_expanded {
                        tree.set_collapsed(&ctx.item_path);
                    } else if let Some(parent_node) = parent_node {
                        let parent_node_path = parent_node.path.clone();

                        // Otherwise, if the current node is collapsed, go to the parent node (directory), select it,
                        // and collapse it.
                        let is_root = tree
                            .root_path
                            .as_ref()
                            .is_some_and(|r| r == &parent_node_path);

                        if !is_root {
                            tree.set_current_path(&parent_node_path);
                            tree.set_collapsed(&parent_node_path);
                        }
                    }
                }
                // Handles the 'Right' navigation event.
                //
                // If a valid node is selected:
                // - If it's a directory:
                //    - Expanded: Move to the first child node and select it.
                //    - Collapsed: Set to expanded.
                Right => {
                    // If the current node is collapsed, expand it.
                    if !ctx.item_is_expanded {
                        tree.set_expanded(&ctx.item_path);
                    } else if let Ok(Some(first_child_path)) =
                        ith_node_path(tree, NodePathOptions::First, ctx.item_path.clone())
                        && !tree.path_is_empty(&ctx.item_path)
                    {
                        // Otherwise, if the current node is expanded and it has children, move to the
                        // first child and select it.
                        tree.set_current_path(first_child_path);
                    }
                }
                // Handles the 'Up' navigation event.
                //
                // Attempts to move the selection to the previous node in the tree. If at the
                // very first node, it wraps around to the end of the tree.
                Up => {
                    let current_node_path = ctx.item_path.clone();
                    let first_node_path =
                        ith_node_path(tree, NodePathOptions::First, root_path.clone());
                    let last_node_path = ith_node_path(tree, NodePathOptions::Last, root_path);

                    // If at the top of the tree, wrap to the end of the tree.
                    if let Ok(Some(first_node_path)) = first_node_path
                        && let Ok(Some(last_node_path)) = last_node_path
                        && current_node_path == first_node_path
                    {
                        tree.set_current_path(last_node_path);
                    } else if let Some(previous_node) = tree.prev(current_node_path) {
                        // Otherwise, select the previous node.
                        tree.set_current_path(previous_node.path);
                    }
                }
                // Handles the 'Down' navigation event.
                //
                // Attempts to move the selection to the next node in the tree. If at the
                // very last node, it wraps around to the beginning of the tree.
                Down => {
                    let current_node_path = ctx.item_path.clone();
                    let first_node_path =
                        ith_node_path(tree, NodePathOptions::First, root_path.clone());
                    let last_node_path = ith_node_path(tree, NodePathOptions::Last, root_path);

                    // If at the bottom of the tree, wrap to the top of the tree.
                    if let Ok(Some(first_node_path)) = first_node_path
                        && let Ok(Some(last_node_path)) = last_node_path
                        && current_node_path == last_node_path
                    {
                        tree.set_current_path(first_node_path);
                    } else if let Some(next_node) = tree.next(current_node_path) {
                        // Otherwise, select the next node.
                        tree.set_current_path(next_node.path);
                    }
                }
            }
        }
        // Handles the 'Toggle Select' event.
        //
        // If a valid node is focused, it is selected/deselected.
        FileExplorerCommand::ToggleSelect => {
            select_first_node_if_current_empty();

            let current_node_path = ctx.item_path.clone();
            tree.toggle_select_for_path(current_node_path);
        }
        // Handles the 'Action' event.
        //
        // If a valid node is selected:
        // - If it's a directory, the expansion state is toggled.
        // - If it's a file, it's marked to be opened.
        FileExplorerCommand::Action => {
            select_first_node_if_current_empty();

            let current_node_path = ctx.item_path.clone();

            if ctx.item_is_directory {
                tree.toggle_expanded(current_node_path);

                ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                    path: target_directory,
                });
            } else {
                ui_intents.push(FileExplorerUiIntent::OpenFile {
                    path: current_node_path,
                });
            }
        }
        // Handles the 'Action Index' event.
        //
        // Functionally the same as the 'Action' event, but instead of using the current node, an
        // index is provided.
        FileExplorerCommand::ActionIndex => {
            let index = ctx.index;
            let children = tree.visible_nodes();
            let index_usize = index as usize;

            // Clear the selected node if click is not on a node or if the index is negative.
            if index_usize >= children.len() || index.is_negative() {
                tree.clear_current_path();
            } else if let Some(target_node) = children.get(index_usize) {
                let target_node_path = target_node.path.clone();
                tree.set_current_path(&target_node_path);

                if target_node.is_dir {
                    tree.toggle_expanded(&target_node_path);

                    ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                        path: target_node_path,
                    });
                } else {
                    ui_intents.push(FileExplorerUiIntent::OpenFile {
                        path: target_node_path.clone(),
                    });
                }
            }
        }
        // Handles the 'Clear Selected' event.
        //
        // First clears the selected node(s) (if any), then clears the current node if no nodes are
        // selected.
        FileExplorerCommand::ClearSelected => {
            if tree.has_selected_nodes() {
                tree.clear_selected_paths();
            } else {
                tree.clear_current_path();
            }
        }
    }

    Ok(FileExplorerCommandResult {
        intents: ui_intents,
    })
}

pub fn get_available_file_explorer_commands(
    ctx: FileExplorerContext,
) -> io::Result<Vec<FileExplorerCommand>> {
    Ok(FileExplorerCommand::all(&ctx))
}

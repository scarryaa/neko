use std::{
    ffi::{CStr, CString, c_char},
    ptr,
};

use crate::{FileNode, FileTree, text::Editor};

pub struct NekoEditor {
    editor: Editor,
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_new() -> *mut NekoEditor {
    let editor = Box::new(NekoEditor {
        editor: Editor::new(),
    });
    Box::into_raw(editor)
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_free(editor: *mut NekoEditor) {
    if !editor.is_null() {
        unsafe {
            let _ = Box::from_raw(editor);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_left(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_left();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_right(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_right();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_up(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_up();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_down(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_down();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_text(
    editor: *mut NekoEditor,
    text: *const c_char,
    len: usize,
) {
    if editor.is_null() || text.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        let text_slice = std::slice::from_raw_parts(text as *const u8, len);
        if let Ok(text_str) = std::str::from_utf8(text_slice) {
            editor.editor.insert_text(text_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_newline(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.insert_newline();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_tab(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.insert_tab();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_backspace(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.backspace();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_delete(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.delete();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_text(
    editor: *const NekoEditor,
    out_len: *mut usize,
) -> *mut c_char {
    if editor.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let editor = &*editor;
        let text = editor.editor.buffer().get_text();

        if !out_len.is_null() {
            *out_len = text.len();
        }

        match CString::new(text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_line(
    editor: *const NekoEditor,
    line_idx: usize,
    out_len: *mut usize,
) -> *mut c_char {
    if editor.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let editor = &*editor;
        let line_text = editor.editor.buffer().get_line(line_idx);

        if !out_len.is_null() {
            *out_len = line_text.len();
        }

        match CString::new(line_text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_line_count(
    editor: *const NekoEditor,
    out_line_count: *mut usize,
) {
    if editor.is_null() {
        if !out_line_count.is_null() {
            unsafe {
                *out_line_count = 0;
            }
        }
        return;
    }

    unsafe {
        let editor = &*editor;
        if !out_line_count.is_null() {
            *out_line_count = editor.editor.buffer().line_count();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_cursor_position(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let cursor = editor.editor.cursor();

        if !out_row.is_null() {
            *out_row = cursor.get_row();
        }
        if !out_col.is_null() {
            *out_col = cursor.get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_all(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_all();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_left(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_left();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_right(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_right();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_up(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_up();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_down(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_down();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_clear_selection(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.clear_selection();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_start(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();

        if !out_row.is_null() {
            *out_row = selection.start().get_row();
        }
        if !out_col.is_null() {
            *out_col = selection.start().get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_end(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();

        if !out_row.is_null() {
            *out_row = selection.end().get_row();
        }
        if !out_col.is_null() {
            *out_col = selection.end().get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_active(editor: *const NekoEditor) -> bool {
    if editor.is_null() {
        return false;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();
        selection.is_active()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_paste(editor: *mut NekoEditor, text: *const c_char, len: usize) {
    if !editor.is_null() && !text.is_null() {
        unsafe {
            let editor = &mut *editor;
            let text_slice = std::slice::from_raw_parts(text as *const u8, len);
            if let Ok(text_str) = std::str::from_utf8(text_slice) {
                editor.editor.insert_text(text_str);
            }
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_copy(editor: *const NekoEditor, out_len: *mut usize) -> *mut c_char {
    if editor.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();
        let buffer = editor.editor.buffer();

        let text = editor.editor.buffer().get_text_range(
            selection.start().get_idx(buffer),
            selection.end().get_idx(buffer),
        );

        if !out_len.is_null() {
            *out_len = text.len();
        }

        match CString::new(text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_string_free(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            let _ = CString::from_raw(s);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_new(root_path: *const c_char) -> *mut FileTree {
    if root_path.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let c_str = CStr::from_ptr(root_path);
        let path_str = match c_str.to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null_mut(),
        };

        match FileTree::new(path_str) {
            Ok(tree) => Box::into_raw(Box::new(tree)),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_free(tree: *mut FileTree) {
    if !tree.is_null() {
        unsafe {
            let _ = Box::from_raw(tree);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_children(
    tree: *mut FileTree,
    path: *const c_char,
    out_nodes: *mut *const FileNode,
    out_count: *mut usize,
) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        let c_str = CStr::from_ptr(path);

        if let Ok(path_str) = c_str.to_str() {
            let children = tree.get_children(path_str);

            if !out_nodes.is_null() {
                *out_nodes = children.as_ptr();
            }

            if !out_count.is_null() {
                *out_count = children.len();
            }
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_next(
    tree: *mut FileTree,
    current_path: *const c_char,
) -> *const FileNode {
    if tree.is_null() || current_path.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;
        let path = match CStr::from_ptr(current_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null(),
        };

        tree.next(path)
            .map(|node| node as *const FileNode)
            .unwrap_or(ptr::null())
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_prev(
    tree: *mut FileTree,
    current_path: *const c_char,
) -> *const FileNode {
    if tree.is_null() || current_path.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;
        let path = match CStr::from_ptr(current_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null(),
        };

        tree.prev(path)
            .map(|node| node as *const FileNode)
            .unwrap_or(ptr::null())
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_toggle_select(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.toggle_select(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_set_current(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.set_current(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_current(tree: *const FileTree) -> *const c_char {
    if tree.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;

        match tree.get_current() {
            Some(path_buf) => match path_buf.to_str() {
                Some(path_str) => match CString::new(path_str) {
                    Ok(cstr) => cstr.into_raw(),
                    Err(_) => ptr::null(),
                },
                None => ptr::null(),
            },
            None => ptr::null(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_is_selected(tree: *mut FileTree, path: *const c_char) -> bool {
    if tree.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let tree = &*tree;
        CStr::from_ptr(path)
            .to_str()
            .map(|p| tree.is_selected(p))
            .unwrap_or(false)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_is_current(tree: *mut FileTree, path: *const c_char) -> bool {
    if tree.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let tree = &*tree;
        CStr::from_ptr(path)
            .to_str()
            .map(|p| tree.is_current(p))
            .unwrap_or(false)
    }
}
